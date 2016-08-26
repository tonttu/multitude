include(../../multitude.pri)

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
  DEFINES += UNITTEST_WIN32_DLL
} else {
  HEADERS += Posix/TimeHelpers.h
  HEADERS += Posix/SignalTranslator.h
  SOURCES += Posix/TimeHelpers.cpp
  SOURCES += Posix/SignalTranslator.cpp
}
DEFINES += UNITTEST_USE_CUSTOM_STREAMS

EXTRA_SOURCES += unittest-cpp.pro README

include(../ThirdParty.pri)
