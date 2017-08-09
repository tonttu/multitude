#include "MultiTactionTestRunner.h"
#include "UnitTest++.h"
#include "XmlTestReporter.h"
#include "TestReporterStdout.h"

#include <Radiant/Trace.hpp>
#include <Radiant/TraceSeverityFilter.hpp>
#include <Radiant/Timer.hpp>
#include <Radiant/StringUtils.hpp>
#include <Radiant/CallStack.hpp>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QProcess>
#include <QDomDocument>
#include <QFile>
#include <QMap>
#include <QTextStream>

#include <fstream>
#include <functional>
#include <cstdio>
#include <mutex>
#include <memory>
#include <cassert>

#ifdef RADIANT_WINDOWS
#include <windows.h>
#include <excpt.h>
#include <DbgHelp.h>
#endif

namespace {

#ifdef RADIANT_LINUX

  void printStackTrace(int, siginfo_t*, void*)
  {
    fprintf(stderr, "CAUGHT UNHANDLED EXCEPTION:\n");

    // This is not a very robust way of getting the stack trace since the class
    // was not designed to be used when the application has crashed. However, it
    // usually gives useful output.
    Radiant::CallStack callStack;
    callStack.print();
  }

  int runTestWithCrashReporting(UnitTest::TestRunner& runner, const std::function<bool (const UnitTest::Test *const)>& predicate)
  {
    struct sigaction printStackTraceSigAction;
    memset(&printStackTraceSigAction, 0, sizeof(printStackTraceSigAction));

    printStackTraceSigAction.sa_flags = SA_SIGINFO | SA_RESETHAND;
    printStackTraceSigAction.sa_sigaction = printStackTrace;

    sigaction(SIGSEGV, &printStackTraceSigAction, NULL);
    sigaction(SIGABRT, &printStackTraceSigAction, NULL);

    // Run the test(s)
    int ret = runner.RunTestsIf(UnitTest::Test::GetTestList(), NULL, predicate, 0);

    return ret;
  }

#endif

#ifdef RADIANT_OSX
  /// @todo Radiant::CallStack::print seems to be really unreliable in
  ///       signal handler on OS X, and often calling it will just close
  ///       the application with return value of zero, which confuses
  ///       test runner. Disable crash reporting on OS X for now.
  int runTestWithCrashReporting(UnitTest::TestRunner& runner, const std::function<bool (const UnitTest::Test *const)>& predicate)
  {
    return runner.RunTestsIf(UnitTest::Test::GetTestList(), NULL, predicate, 0);
  }
#endif

#ifdef RADIANT_WINDOWS
  int filterExceptionPrintStackTrace(int exceptionCode, PEXCEPTION_POINTERS exceptionPointers)
  {
    fprintf(stderr, "CAUGHT UNHANDLED EXCEPTION:\n");
    (void)exceptionCode;

    HANDLE currentProcess = GetCurrentProcess();

    // Initialize the symbol handler for current process
    if(SymInitialize(currentProcess, NULL, TRUE) == FALSE) {
      fprintf(stderr, "printStackTrace # SymInitialize failed: %s\n", Radiant::StringUtils::getLastErrorMessage().toUtf8().data());
      return EXCEPTION_EXECUTE_HANDLER;
    }

    // Define symbol loading options
    DWORD symbolOptions = SymGetOptions();
    symbolOptions |= SYMOPT_LOAD_LINES; // Want line numbers
    SymSetOptions(symbolOptions);

    // Initialize stack frame based on where the exception occurred (only works
    // on amd64 architecture)
    STACKFRAME64 stackFrame;

    // Setup instruction pointer
    stackFrame.AddrPC.Offset = exceptionPointers->ContextRecord->Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;

    // Setup stack pointer
    stackFrame.AddrStack.Offset = exceptionPointers->ContextRecord->Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;

    // Setup frame pointer
    stackFrame.AddrFrame.Offset = exceptionPointers->ContextRecord->Rbp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;

    const int maxFrames = 32;
    int frameNumber = 0;

    while(frameNumber++ < maxFrames) {

      // May fail, in that case we just abort
      if(!StackWalk64(IMAGE_FILE_MACHINE_AMD64, currentProcess, GetCurrentThread(), &stackFrame, exceptionPointers->ContextRecord, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
        break;

      // May fail, in that case we just abort
      if(stackFrame.AddrPC.Offset == 0)
        break;

      const int maxSymbolNameLen = 1024;
      const int symbolBytesToAllocate = sizeof(SYMBOL_INFO) + (maxSymbolNameLen - 1) * sizeof(TCHAR);

      // Initialize symbol struct
      SYMBOL_INFO* symbol = static_cast<SYMBOL_INFO*>(malloc(symbolBytesToAllocate));
      memset(symbol, 0, symbolBytesToAllocate);
      symbol->MaxNameLen = maxSymbolNameLen;
      symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

      // Find the symbol name.
      // Don't check for errors, we output empty symbol name below if this
      // fails, but we might be able to still continue traversing the stack
      DWORD64 symbolDisplacement = 0;
      SymFromAddr(currentProcess, stackFrame.AddrPC.Offset, &symbolDisplacement, symbol);

      // Find the line number
      IMAGEHLP_LINE64 line = {};
      line.SizeOfStruct = sizeof(line);
      DWORD lineOffset = 0;

      // Don't check for errors, this will always fail for RtlUserThreadStart
      // and BaseThreadInitThunk. The line numbers for those will just appear
      // as 0.
      SymGetLineFromAddr64(currentProcess, stackFrame.AddrPC.Offset, &lineOffset, &line);

      // Output whatever we managed to find out regardless of failures above,
      // it might still be useful
      fprintf(stderr, "#%d %s at %s:%d\n", frameNumber, symbol->Name, line.FileName, line.LineNumber);
    }

    return EXCEPTION_EXECUTE_HANDLER;
  }

  // In order to use structured exception handling (SEH), we can not have have
  // object unwinding (destruction) in the function that has the __try {}
  // __except keywords. This function isolates any destruction from the SEH
  // handler. See https://msdn.microsoft.com/en-us/library/xwtb73ad.aspx
  int runTestWithCrashReporting(UnitTest::TestRunner& runner, const std::function<bool (const UnitTest::Test *const)>& predicate)
  {
    __try {

      return runner.RunTestsIf(UnitTest::Test::GetTestList(), NULL, predicate, 0);

    } __except(filterExceptionPrintStackTrace(GetExceptionCode(), GetExceptionInformation())) {
    }

    abort();

    // Just to keep the compiler happy
    return 1;
  }
#endif

}

namespace UnitTest
{

  namespace
  {
    static std::mutex s_argumentsMutex;
    static QStringList s_arguments;

    void writeFailureXml(const QString & path, const UnitTest::Test * test, double time, const QString & message)
    {
      auto xmlContents = QString("<?xml version=\"1.0\"?>"
        "<unittest-results tests=\"1\" failedtests=\"1\" failures=\"1\" time=\"%1\">\n"
          "<test suite=\"%2\" name=\"%3\" time=\"%1\">"
            "<failure message=\"%4\"/>"
          "</test>"
        "</unittest-results>").arg(time).arg(test->m_details.suiteName)
          .arg(test->m_details.testName).arg(message);
      QFile file(path);
      if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << xmlContents;
        file.close();
      }
    }

    int mergeXml(QDomDocument & doc, const QString & filename)
    {
      QFile file(filename);
      if (!file.open(QFile::ReadOnly)) {
        fprintf(stderr, "mergeXml # Failed to open %s: %s\n",
                filename.toUtf8().data(),
                file.errorString().toUtf8().data());
        return 1;
      }

      if (doc.isNull() || doc.documentElement().isNull()) {
        // Replace the whole document if it's empty.
        //
        // It could be non-null and missing a root element
        // at least if setContent() has been called with
        // an empty xml file, so handle that too.
        doc.setContent(&file);
      } else {
        QDomDocument importDoc;
        importDoc.setContent(&file);
        QDomElement importRoot = importDoc.documentElement();
        QDomElement root = doc.documentElement();

        // copy all nodes
        QDomNode n = importRoot.firstChild();
        while (!n.isNull()) {
          root.appendChild(doc.importNode(n, true));
          n = n.nextSibling();
        }

        // copy all attributes, sum numeric attributes (failures, tests, failedtests, time)
        QDomNamedNodeMap importAttrs = importRoot.attributes();
        for (int i = 0; i < importAttrs.count(); ++i) {
          QDomAttr attr = importAttrs.item(i).toAttr();
          if (attr.isNull())
            continue;
          if (root.hasAttribute(attr.name())) {
            bool ok;
            double a = root.attribute(attr.name()).toDouble(&ok);
            if (!ok)
              continue;
            double b = attr.value().toDouble(&ok);
            if (!ok)
              continue;
            root.setAttribute(attr.name(), QString::number(a+b));
          } else {
            root.setAttribute(attr.name(), attr.value());
          }
        }
      }
      return 0;
    }

    int listTests()
    {
      auto test = UnitTest::Test::GetTestList().GetHead();
      int i = 0;
      while (test) {
        printf("%d\t%s/%s\n", ++i, test->m_details.suiteName, test->m_details.testName);
        test = test->m_nextTest;
      }
      return 0;
    }

    int runOneTest(int index, int count, const UnitTest::Test* const test, QString xmlOutput,
                   const char *procName, bool verbose, bool silent)
    {
      QProcess process;
      process.setProcessChannelMode(QProcess::ForwardedChannels);
      QStringList newArgs;
      QString singleArg = test->m_details.suiteName;
      singleArg += "/";
      singleArg += test->m_details.testName;

      if(silent) newArgs << "-s";
      if(verbose) newArgs << "-v";

      newArgs << "--single" << singleArg << xmlOutput;
      if (QFile::exists(xmlOutput))
        QFile::remove(xmlOutput);

      printf("%2d/%2d: Running test %s/%s\n",
             index, count, test->m_details.suiteName, test->m_details.testName);

      // If the test crashes or hangs, get an estimate of the time used with a timer
      Radiant::Timer timer;

      process.start(procName, newArgs, QProcess::ReadOnly);
      process.waitForStarted();
      bool finished = process.waitForFinished(15 * 60 * 1000);
      const double time = timer.time();
      if (!finished) {
        printf("Test %s/%s timed out. See %s for details.\n",
               test->m_details.suiteName, test->m_details.testName, xmlOutput.toUtf8().data());
        writeFailureXml(xmlOutput, test, time, "timeout");
        process.close();
        return 1;
      }

      if(process.exitStatus() == QProcess::CrashExit) {
        printf("Test %s/%s crashed. See %s for details.\n",
               test->m_details.suiteName, test->m_details.testName, xmlOutput.toUtf8().data());
        writeFailureXml(xmlOutput, test, time, "crashed");
        return 1;
      }

      // exitCode() is only valid on normal process exit
      assert(process.exitStatus() == QProcess::NormalExit);

      int procExitCode = process.exitCode();
      if(procExitCode) {
        printf("Test %s/%s failed. See %s for details.\n",
               test->m_details.suiteName, test->m_details.testName, xmlOutput.toUtf8().data());
#ifdef RADIANT_UNIX
        int r = system("uptime");
        r += system("ps aux");
        (void)r;
#endif
      }
      return procExitCode;
    }

    std::vector<const UnitTest::Test*> includeTests(QString match)
    {
      std::vector<const UnitTest::Test*> toRun;
      QRegExp matchRegex(match);
      auto test = UnitTest::Test::GetTestList().GetHead();
      while (test) {
        QString testName = test->m_details.testName;
        QString suiteName = test->m_details.suiteName;
        QString matchCandidate = suiteName + "/" + testName;
        if(matchRegex.isEmpty() || matchCandidate.contains(matchRegex)) {
          toRun.push_back(test);
        }
        test = test->m_nextTest;
      }
      return toRun;
    }

    std::vector<const UnitTest::Test*> excludeTests(QString match)
    {
      std::vector<const UnitTest::Test*> toRun;

      auto test = UnitTest::Test::GetTestList().GetHead();
      while (test) {
        QString testName = test->m_details.testName;
        QString suiteName = test->m_details.suiteName;
        QString matchCandidate = suiteName + "/" + testName;

        if(!matchCandidate.contains(match)) {
          toRun.push_back(test);
        }
        test = test->m_nextTest;
      }
      return toRun;
    }

    void printTestReport(const QDomDocument & doc)
    {
      auto root = doc.documentElement();
      int failedTests = root.attribute("failedtests").toInt();
      int tests = root.attribute("tests").toInt();
      int failures = root.attribute("failures").toInt();
      float time = root.attribute("time").toFloat();

      int secs = time;
      int mins = secs / 60;
      secs = secs % 60;
      printf("Ran %d test in %d min %d s\n", tests, mins, secs);
      if (failedTests == 0 && failures == 0) {
        printf("No errors\n");
      } else {
        printf("FAILED - %d failed tests, %d errors\n", failedTests, failures);
        auto testElements = root.elementsByTagName("test");
        for (int i = 0; i < testElements.size(); ++i) {
          QDomElement e = testElements.item(i).toElement();
          auto failureElements = e.elementsByTagName("failure");
          if (!failureElements.isEmpty()) {
            printf("\n%s/%s [%.3f s]: %d %s:\n", e.attribute("suite").toUtf8().data(),
                   e.attribute("name").toUtf8().data(), e.attribute("time").toFloat(),
                   failureElements.size(), failureElements.size() == 1 ? "error" : "errors");
            QMap<QString, int> count;
            QStringList messages;
            for (int j = 0; j < failureElements.size(); ++j) {
              auto msg = failureElements.item(j).toElement().attribute("message");
              if (count[msg]++ == 0) {
                messages << msg;
              }
            }
            for (auto msg: messages) {
              int c = count[msg];
              if (c > 1) {
                printf("  %s [%d times]\n", msg.toUtf8().data(), c);
              } else {
                printf("  %s\n", msg.toUtf8().data());
              }
            }
          }
        }
      }
    }

    int runTests(const std::vector<const UnitTest::Test*>& toRun, QString xmlOutput, const char *procName,
                 bool verbose, bool silent)
    {
      int count = static_cast<int>(toRun.size());

      if (xmlOutput.isEmpty())
        xmlOutput = "TestTemp.xml";

      QDomDocument dom;
      int index = 0;
      int exitCode = 0;
      for(const UnitTest::Test * const test : toRun) {
        ++index;
        // run the test in a subprocess. Creating a MultiWidgets::Application after one
        // has been destroyed does not work properly.
        int procExitCode = runOneTest(index, count, test, xmlOutput, procName,
                                      verbose, silent);

        if(procExitCode != 0)
          exitCode = procExitCode;

        if(mergeXml(dom, xmlOutput) != 0) {
          Radiant::error("Failed to merge test result XML. Aborting...");
          break;
        }
      }
      if (!xmlOutput.isEmpty()) {
        QFile output(xmlOutput);
        if (!output.open(QFile::WriteOnly)) {
          fprintf(stderr, "Failed to open %s: %s\n",
                  xmlOutput.toUtf8().data(),
                  output.errorString().toUtf8().data());
          return 1;
        }
        output.write(dom.toString().toUtf8());
      }

      printTestReport(dom);

      return exitCode;
    }

    int runSingleTest(QString testName, QString testSuite, QString xmlOutput)
    {
      std::unique_ptr<std::ofstream> xmlStream;
      std::unique_ptr<UnitTest::TestReporter> reporter;
      if(!xmlOutput.isEmpty()) {
        xmlStream.reset(new std::ofstream(xmlOutput.toUtf8().data()));
        reporter.reset(new UnitTest::XmlTestReporter(*xmlStream));
      } else {
        reporter.reset(new UnitTest::TestReporterStdout());
      }
      UnitTest::TestRunner runner(*reporter);
      int foundCount = 0;
      auto predicate = [&foundCount, testName, testSuite](const UnitTest::Test *const test) {
        bool found = testName == test->m_details.testName
            && (testSuite.isEmpty() || testSuite == test->m_details.suiteName);
        if(found) {
          foundCount++;
        }
        return found;
      };

      int errorCode = runTestWithCrashReporting(runner, predicate);

      if(foundCount == 0) {
        fprintf(stderr, "Failed to find test '%s' in suite '%s'\n",
                testName.toUtf8().data(),
                testSuite.toUtf8().data());
        return 1;
      }
      if(foundCount > 1) {
        fprintf(stderr, "Found more than one test with name '%s' in suite '%s'\n",
                testName.toUtf8().data(),
                testSuite.toUtf8().data());
        return 1;
      }
      return errorCode;
    }

    template <typename T>
    T repeat(T vector, int times)
    {
      auto len = vector.size();
      for (int i = 1; i < times; ++i) {
        for (std::size_t j = 0; j < len; ++j) {
          vector.push_back(vector[j]);
        }
      }
      return vector;
    }

    int runTestsInclusive(QString match, QString xmlOutput, const char *procName, bool verbose, bool silent, int times)
    {
      std::vector<const UnitTest::Test*> toRun = repeat(includeTests(match), times);

      if(toRun.empty()) {
        fprintf(stderr, "Failed to find tests with name or suite matching %s\n",
                match.toUtf8().data());
        return 1;
      }

      return runTests(toRun, xmlOutput, procName, verbose, silent);
    }

    int runTestsExclusive(QString match, QString xmlOutput, const char *procName, bool verbose, bool silent, int times)
    {
      std::vector<const UnitTest::Test*> toRun = repeat(excludeTests(match), times);

      if(toRun.empty()) {
        fprintf(stderr, "Failed to find tests with name or suite matching %s\n",
                match.toUtf8().data());
        return 1;
      }

      return runTests(toRun, xmlOutput, procName, verbose, silent);
    }

  }  // unnamed namespace

  int runTests(int argc, char ** argv)
  {
    auto app = new QCoreApplication(argc, argv);

    /// Avoid number separator mess
    setlocale(LC_NUMERIC, "C");

    QStringList cmdLineArgs;
    for(int i = 0; i < argc; ++i) {
      cmdLineArgs << QString(argv[i]);
    }
    {
      std::lock_guard<std::mutex> guard(s_argumentsMutex);
      s_arguments = cmdLineArgs;
    }

    QCommandLineParser parser;
    QCommandLineOption singleOption("single",
                                    "Run a single test without creating a subprocess.",
                                    "TEST_NAME");
    QCommandLineOption listOption("list", "List all available tests.");
    QCommandLineOption matchOption("match",
                                   "Run only the tests that match the given regex.",
                                   "REGEX");

    QCommandLineOption excludeOption("exclude", "Pattern to exclude tests (not regexp)", "PATTERN");

    QCommandLineOption v("v", "Verbose mode.");
    QCommandLineOption s("s", "Silent mode, suppress all console output from Cornerstone");

    QCommandLineOption timesOption("times", "Repeat the tests multiple times, doesn't work with --single", "NUMBER");

    parser.addOptions({singleOption, listOption, matchOption, excludeOption, v, s, timesOption });
    parser.addPositionalArgument("xmlFile", "XML file for the test status output");
    parser.addHelpOption();
    parser.process(cmdLineArgs);

    QStringList positional = parser.positionalArguments();
    if (positional.size() > 1) {
      positional.removeFirst();
      fprintf(stderr, "Found extra command line arguments: %s\n",
              positional.join(" ").toUtf8().data());
      Radiant::Trace::initialize();
      return 1;
    }
    QString xmlOutput = positional.empty() ? QString() : positional.front();

    if (parser.isSet("list")) {
      Radiant::Trace::initialize();
      return listTests();
    }

    bool silent = parser.isSet("s");
    bool verbose = parser.isSet("v");

    if(silent) {
      auto dropAllMessages = [] (const Radiant::Trace::Message &) { return true; };
      Radiant::Trace::addFilter(dropAllMessages, Radiant::Trace::Filter::ORDER_DEFAULT_FILTERS);
      Radiant::Trace::initialize(Radiant::Trace::INIT_NO_FLAGS);
    } else if(verbose) {
      Radiant::Trace::findOrCreateFilter<Radiant::Trace::SeverityFilter>()->setMinimumSeverityLevel(Radiant::Trace::DEBUG);
      Radiant::Trace::initialize();
    } else {
      Radiant::Trace::initialize();
    }

    const QString single = parser.value("single");
    const QString include = parser.value("match");
    const QString exclude = parser.value("exclude");
    const int times = parser.value("times").toInt();

    delete app;
    if(!single.isEmpty()) {

      if(!include.isEmpty() || !exclude.isEmpty()) {
        fprintf(stderr, "Don't specify --match or --exclude with --single");
        return 1;
      }

      QStringList parts = single.split("/");
      QString name, suite;
      if(parts.size() == 1) {
        name = parts[0];
      }
      if(parts.size() == 2) {
        suite = parts[0];
        name = parts[1];
      }
      if(parts.size() == 0 || parts.size() > 2) {
        fprintf(stderr, "Invalid argument to --single. Expecting suiteName/testName or just testName\n");
        return 1;
      }
      return runSingleTest(name, suite, xmlOutput);
    } else if(!exclude.isEmpty()) {

      if(!include.isEmpty()) {
        fprintf(stderr, "Don't specify --match with --exclude");
        return 1;
      }

      return runTestsExclusive(exclude, xmlOutput, argv[0], verbose, silent, times);

    } else {
      return runTestsInclusive(include, xmlOutput, argv[0], verbose, silent, times);
    }
  }

  QStringList getCommandLineArgs()
  {
    std::lock_guard<std::mutex> guard(s_argumentsMutex);
    return s_arguments;
  }

}
