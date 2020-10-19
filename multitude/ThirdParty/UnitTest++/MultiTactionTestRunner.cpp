#include "MultiTactionTestRunner.h"
#include "UnitTest++.h"
#include "XmlTestReporter.h"
#include "TestReporterStdout.h"
#include "CompositeTestReporter.h"

#include <Radiant/BGThread.hpp>
#include <Radiant/Semaphore.hpp>
#include <Radiant/TemporaryDir.hpp>
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
#include <QSettings>

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

namespace
{
  class RadiantTraceReporter : public UnitTest::TestReporter
  {
  private:
    virtual void ReportTestStart(UnitTest::TestDetails const &)
    {}

    virtual void ReportFailure(UnitTest::TestDetails const & test, char const * failure)
    {
      Radiant::error("%s:%d: Failure in %s: %s", test.filename, test.lineNumber, test.testName, failure);
    }

    virtual void ReportTestFinish(UnitTest::TestDetails const &, float)
    {}

    virtual void ReportSummary(int totalTestCount, int failedTestCount, int failureCount, float secondsElapsed)
    {
      if (failureCount > 0)
        Radiant::error("%d out of %d tests failed (%d failures).", failedTestCount, totalTestCount, failureCount);
      else
        Radiant::info("Success: %d tests passed.", totalTestCount);

      Radiant::info("Test time: %.2f seconds.", secondsElapsed);
    }
  };

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
      fprintf(stderr, "#%d %s at %s:%lu\n", frameNumber, symbol->Name, line.FileName, line.LineNumber);
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

enum class TestRunnerFlag
{
  VERBOSE              = 1 << 0,
  SILENT               = 1 << 1,
  PRINT_ONLY_FAILURES  = 1 << 2,
  PARALLEL             = 1 << 3,
};
MULTI_FLAGS(TestRunnerFlag)
typedef Radiant::FlagsT<TestRunnerFlag> TestRunnerFlags;

struct RunOptions
{
  TestRunnerFlags flags;
  QStringList wrapper;
  double timeoutSeconds = 5*60.0;
};

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
            if (attr.name() == "time") {
              double a = root.attribute(attr.name()).toDouble(&ok);
              if (!ok)
                continue;
              double b = attr.value().toDouble(&ok);
              if (!ok)
                continue;
              root.setAttribute(attr.name(), QString::number(a+b));
            } else {
              qlonglong a = root.attribute(attr.name()).toLongLong(&ok);
              if (!ok)
                continue;
              qlonglong b = attr.value().toLongLong(&ok);
              if (!ok)
                continue;
              root.setAttribute(attr.name(), QString::number(a+b));
            }
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
                   const char *procName, RunOptions options)
    {
      QProcess process;
      if (options.flags & TestRunnerFlag::PRINT_ONLY_FAILURES) {
        process.setProcessChannelMode(QProcess::MergedChannels);
      } else {
        process.setProcessChannelMode(QProcess::ForwardedChannels);
      }
      QStringList newArgs;
      QString singleArg = test->m_details.suiteName;
      singleArg += "/";
      singleArg += test->m_details.testName;

      if (options.flags & TestRunnerFlag::SILENT) newArgs << "-s";
      if (options.flags & TestRunnerFlag::VERBOSE) newArgs << "-v";

      newArgs << "--single" << singleArg << xmlOutput;
      if (QFile::exists(xmlOutput))
        QFile::remove(xmlOutput);

      printf("%d/%d: Running test %s/%s\n",
             index, count, test->m_details.suiteName, test->m_details.testName);

      QString command;
      if (options.wrapper.isEmpty()) {
        command = procName;
      } else {
        command = options.wrapper[0];
        newArgs = (options.wrapper.mid(1) << procName) + newArgs;
      }

      // If the test crashes or hangs, get an estimate of the time used with a timer
      Radiant::Timer timer;

      process.start(command, newArgs, QProcess::ReadOnly);
      process.waitForStarted();
      bool finished = process.waitForFinished(options.timeoutSeconds * 1000.0);
      const double time = timer.time();

      QString error, errorDetails;
      int exitCode = 0;

      bool validOutput = false;
      {
        QFile file(xmlOutput);
        validOutput = file.open(QFile::ReadOnly) && !file.readAll().trimmed().isEmpty();
      }
      if (!finished) {
        error = "timed out";
        writeFailureXml(xmlOutput, test, time, "timeout");
        exitCode = 1;
      } else if (process.exitStatus() == QProcess::CrashExit) {
        error = "crashed";
        writeFailureXml(xmlOutput, test, time, "crashed");
        exitCode = 1;
      } else if (!validOutput && process.exitCode() != 0) {
        // In Windows when the application calls abort() in debug mode, the
        // application doesn't technically crash, so QProcess exit status is
        // NormalExit, but the application still fails to write the output
        // xml file. Make sure we don't miss this failure.
        error = "failed";
        writeFailureXml(xmlOutput, test, time, "failed");
        exitCode = process.exitCode();
      } else {
        // exitCode() is only valid on normal process exit
        assert(process.exitStatus() == QProcess::NormalExit);
        exitCode = process.exitCode();

        if (exitCode) {
          QDomDocument doc;
          mergeXml(doc, xmlOutput);
          QDomNodeList failures = doc.documentElement().firstChild().childNodes();
          for (int i = 0; i < failures.length(); ++i) {
            const QString msg = failures.at(i).toElement().attribute("message");
            if (!msg.isEmpty())
              errorDetails += "  " + msg + "\n";
          }
          error = "failed";
        }
      }

      QSettings stats("MultiTaction", "TestRunner");
      QString statsKey = QString("times/%1.%2").arg(test->m_details.suiteName, test->m_details.testName);

      if (exitCode) {
        printf("\nTest %s/%s %s after %.2lfs%s%s\n",
               test->m_details.suiteName, test->m_details.testName, error.toUtf8().data(),
               time, errorDetails.isEmpty() ? "" : ":\n", errorDetails.toUtf8().data());

        if (options.flags & TestRunnerFlag::PRINT_ONLY_FAILURES)
          printf("Application output:\n%s\n", process.readAll().replace("\r\n", "\n").data());

        // We remove time stat for a failed test to force it to run first next time
        stats.remove(statsKey);
      } else {
        // Next time these stats are used to order the test list so that slower tests
        // are executed first, which will optimize total wall-clock times when running
        // tests in parallel.
        stats.setValue(statsKey, time);
      }
      process.close();
      fflush(stdout);
      return exitCode;
    }

    std::vector<const UnitTest::Test*> filterTests(const QString & includeR, const QString & excludeR)
    {
      std::multimap<double, const UnitTest::Test*> toRun;
      QRegExp includeRegex(includeR);
      QRegExp excludeRegex(excludeR);

      QSettings stats("MultiTaction", "TestRunner");

      auto test = UnitTest::Test::GetTestList().GetHead();
      while (test) {
        QString testName = test->m_details.testName;
        QString suiteName = test->m_details.suiteName;
        QString matchCandidate = suiteName + "/" + testName;

        if ((includeRegex.isEmpty() || matchCandidate.contains(includeRegex)) &&
            (excludeRegex.isEmpty() || !matchCandidate.contains(excludeRegex))) {
          QString statsKey = QString("times/%1.%2").arg(suiteName, testName);
          double key = stats.value(statsKey, std::numeric_limits<double>::infinity()).toDouble();
          toRun.insert(std::make_pair(-key, test));
        }
        test = test->m_nextTest;
      }
      std::vector<const UnitTest::Test*> ret;
      for (auto & p: toRun)
        ret.push_back(p.second);
      return ret;
    }

    void printTestReport(const QDomDocument & doc, float time)
    {
      auto root = doc.documentElement();
      int failedTests = root.attribute("failedtests").toInt();
      int tests = root.attribute("tests").toInt();
      int failures = root.attribute("failures").toInt();

      int secs = time;
      int mins = secs / 60;
      secs = secs % 60;
      printf("Ran %d test%s in %d min %d s\n", tests, tests == 1 ? "" : "s", mins, secs);
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
                 RunOptions options)
    {
      Radiant::Timer timer;
      int count = static_cast<int>(toRun.size());

      if (xmlOutput.isEmpty())
        xmlOutput = "TestTemp.xml";

      QDomDocument dom;
      Radiant::TemporaryDir tmp;
      int index = 0;
      int exitCode = 0;

      Radiant::BGThread bg("BGThread test");
      if (options.flags & TestRunnerFlag::PARALLEL)
        bg.run(QThread::idealThreadCount() * 1.25);
      else
        bg.run(1);

      Radiant::Mutex domMutex;
      Radiant::Semaphore sem;

      for(const UnitTest::Test * const test : toRun) {
        QString tmpXmlOutput(tmp.path() + QString("/test.%1.xml").arg(index));
        ++index;
        // run the test in a subprocess. Creating a MultiWidgets::Application after one
        // has been destroyed does not work properly.
        auto func = [=, &exitCode, &domMutex, &dom, &sem] (Radiant::Task & task) {
          int procExitCode = runOneTest(index, count, test, tmpXmlOutput, procName, options);

          if(procExitCode != 0)
            exitCode = procExitCode;

          {
            Radiant::Guard g(domMutex);
            if(mergeXml(dom, tmpXmlOutput) != 0) {
              Radiant::error("Failed to merge test result XML. Aborting...");
              exit(1);
            }
          }
          QFile::remove(tmpXmlOutput);
          task.setFinished();
          sem.release(1);
        };

        bg.addTask(std::make_shared<Radiant::FunctionTask>(func));
      }
      sem.acquire(static_cast<int>(toRun.size()));

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

      printTestReport(dom, timer.time());

      return exitCode;
    }

    int runSingleTest(QString testName, QString testSuite, QString xmlOutput)
    {
      std::unique_ptr<std::ofstream> xmlStream;
      std::unique_ptr<UnitTest::CompositeTestReporter> reporter{new UnitTest::CompositeTestReporter()};
      reporter->AddReporter(new RadiantTraceReporter());
      if(!xmlOutput.isEmpty()) {
        xmlStream.reset(new std::ofstream(xmlOutput.toUtf8().data()));
        reporter->AddReporter(new UnitTest::XmlTestReporter(*xmlStream));
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

    int runTests(const QString & match, const QString & exclude, QString xmlOutput, const char *procName, RunOptions options, int times)
    {
      std::vector<const UnitTest::Test*> toRun = repeat(filterTests(match, exclude), times);

      if(toRun.empty()) {
        fprintf(stderr, "Failed to find tests with name or suite matching %s\n",
                match.toUtf8().data());
        return 1;
      }

      return runTests(toRun, xmlOutput, procName, options);
    }

  }  // unnamed namespace

  int runTests(int argc, char ** argv)
  {
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

    QCommandLineOption excludeOption("exclude", "Exclude tests matching this regex. "
        "Exclude takes priority if both --match and --exclude are specified and the parameters conflict.",
        "REGEX");

    QCommandLineOption v("v", "Run individual tests in verbose mode.");
    QCommandLineOption s("s", "Run individual tests in silent mode, suppress all console output from Cornerstone");
    QCommandLineOption printOnlyFailuresOptions("print-only-failures",
                                                "Print individual test output only if the test fails");
    QCommandLineOption parallel("j", "Run tests in parallel");

    QCommandLineOption timesOption("times", "Repeat the tests multiple times, doesn't work with --single", "NUMBER");

    QCommandLineOption wrapperOption("wrapper", "Run all tests through this wrapper command (for instance valgrind, gdb...)."
                                     " Specify multiple times to give arguments to the wrapper command", "COMMANDS");

    QCommandLineOption timeoutOption("timeout", "Timeout for individual tests, in seconds.", "NUMBER");

    QCommandLineOption helpOption({"h", "help"}, "Show this help");

    parser.addOptions({singleOption, listOption, matchOption, excludeOption, v, s,
                       printOnlyFailuresOptions, parallel, timesOption, helpOption,
                       wrapperOption, timeoutOption });
    parser.addPositionalArgument("xmlFile", "XML file for the test status output");
    parser.process(cmdLineArgs);

    if (parser.isSet(helpOption)) {
      QCoreApplication app(argc, argv);
      parser.showHelp();
    }

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

    RunOptions options;
    if (parser.isSet(s))
      options.flags |= TestRunnerFlag::SILENT;
    if (parser.isSet(v))
      options.flags |= TestRunnerFlag::VERBOSE;
    if (parser.isSet(printOnlyFailuresOptions))
      options.flags |= TestRunnerFlag::PRINT_ONLY_FAILURES;

    if (parser.isSet(parallel))
      options.flags |= TestRunnerFlag::PARALLEL;

    if (options.flags & TestRunnerFlag::SILENT) {
      auto dropAllMessages = [] (const Radiant::Trace::Message &) { return true; };
      Radiant::Trace::addFilter(dropAllMessages, Radiant::Trace::Filter::ORDER_DEFAULT_FILTERS);
      Radiant::Trace::initialize(Radiant::Trace::INIT_QT_MESSAGE_HANDLER);
    } else if (options.flags & TestRunnerFlag::VERBOSE) {
      Radiant::Trace::findOrCreateFilter<Radiant::Trace::SeverityFilter>()->setMinimumSeverityLevel(Radiant::Trace::DEBUG);
      Radiant::Trace::initialize();
    } else {
      Radiant::Trace::initialize();
    }

    options.wrapper = parser.values(wrapperOption);

    if (parser.isSet(timeoutOption))
      options.timeoutSeconds = parser.value(timeoutOption).toDouble();

    const QString single = parser.value("single");
    const QString include = parser.value("match");
    const QString exclude = parser.value("exclude");
    const int times = parser.value("times").toInt();

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
    } else {
      return runTests(include, exclude, xmlOutput, argv[0], options, times);
    }
  }

  QStringList getCommandLineArgs()
  {
    std::lock_guard<std::mutex> guard(s_argumentsMutex);
    return s_arguments;
  }

}
