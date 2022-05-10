# There doesn't seem to be any way copy files on Windows that would work with
# spaces and special characters, like + signs. Tried copy, xcopy, robocopy
win32 {
  QMAKE_COPY_FILE='\"C:\Program Files\Git\usr\bin\cp.exe\" -f'
  QMAKE_INSTALL_FILE='\"C:\Program Files\Git\usr\bin\install.exe\"'
}

include(../../setup.pri)

QT += xml

INCLUDEPATH += .

DEFINES += UNITTEST_DLL_EXPORT

HEADERS +=  AssertException.h \
            AssertException.h \
            CheckMacros.h \
            Checks.h \
            CompositeTestReporter.h \
            Config.h \
            CurrentTest.h \
            DeferredTestReporter.h \
            DeferredTestResult.h \
            ExceptionMacros.h \
            ExecuteTest.h \
            HelperMacros.h \
            MemoryOutStream.h \
            MultiTactionTestRunner.h \
            ReportAssert.h \
            ReportAssertImpl.h \
            RequiredCheckException.h \
            RequiredCheckTestReporter.h \
            RequireMacros.h \
            TestDetails.h \
            Test.h \
            TestList.h \
            TestMacros.h \
            TestReporter.h \
            TestReporterStdout.h \
            TestResults.h \
            TestRunner.h \
            TestSuite.h \
            ThrowingTestReporter.h \
            TimeConstraint.h \
            TimeHelpers.h \
            UnitTest++.h \
            UnitTestPP.h \
            XmlTestReporter.h

SOURCES +=  AssertException.cpp \
            Checks.cpp \
            CompositeTestReporter.cpp \
            CurrentTest.cpp \
            DeferredTestReporter.cpp \
            DeferredTestResult.cpp \
            MemoryOutStream.cpp \
            MultiTactionTestRunner.cpp \
            ReportAssert.cpp \
            RequiredCheckException.cpp \
            RequiredCheckTestReporter.cpp \
            Test.cpp \
            TestDetails.cpp \
            TestList.cpp \
            TestReporter.cpp \
            TestReporterStdout.cpp \
            TestResults.cpp \
            TestRunner.cpp \
            ThrowingTestReporter.cpp \
            TimeConstraint.cpp \
            XmlTestReporter.cpp

LIBS += $$LIB_RADIANT

win32 {
  HEADERS += Win32/TimeHelpers.h
  SOURCES += Win32/TimeHelpers.cpp
  DEFINES += UNITTEST_WIN32_DLL _CRT_SECURE_NO_WARNINGS
  LIBS += Dbghelp.lib
} else {
  HEADERS += Posix/TimeHelpers.h
  HEADERS += Posix/SignalTranslator.h
  SOURCES += Posix/TimeHelpers.cpp
  SOURCES += Posix/SignalTranslator.cpp
}
DEFINES += UNITTEST_USE_CUSTOM_STREAMS

include(../ThirdParty.pri)
