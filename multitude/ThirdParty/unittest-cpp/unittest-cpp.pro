include(../../multitude.pri)

QT += xml
TEMPLATE = lib
CONFIG += shared

INCLUDEPATH += .

include(../../library.pri)

DEFINES += UNITTEST_DLL_EXPORT

HEADERS += UnitTest++/AssertException.h \
           UnitTest++/CheckMacros.h \
           UnitTest++/Checks.h \
           UnitTest++/CompositeTestReporter.h \
           UnitTest++/Config.h \
           UnitTest++/CurrentTest.h \
           UnitTest++/DeferredTestReporter.h \
           UnitTest++/DeferredTestResult.h \
           UnitTest++/ExceptionMacros.h \
           UnitTest++/ExecuteTest.h \
           UnitTest++/HelperMacros.h \
           UnitTest++/MemoryOutStream.h \
           UnitTest++/ReportAssert.h \
           UnitTest++/ReportAssertImpl.h \
           UnitTest++/Test.h \
           UnitTest++/TestDetails.h \
           UnitTest++/TestList.h \
           UnitTest++/TestMacros.h \
           UnitTest++/TestReporter.h \
           UnitTest++/TestReporterStdout.h \
           UnitTest++/TestResults.h \
           UnitTest++/TestRunner.h \
           UnitTest++/TestSuite.h \
           UnitTest++/TimeConstraint.h \
           UnitTest++/TimeHelpers.h \
           UnitTest++/UnitTest++.h \
           UnitTest++/UnitTestPP.h \
           UnitTest++/XmlTestReporter.h \
           UnitTest++/MultiTactionTestRunner.h

SOURCES += UnitTest++/AssertException.cpp \
           UnitTest++/Checks.cpp \
           UnitTest++/CompositeTestReporter.cpp \
           UnitTest++/CurrentTest.cpp \
           UnitTest++/DeferredTestReporter.cpp \
           UnitTest++/DeferredTestResult.cpp \
           UnitTest++/MemoryOutStream.cpp \
           UnitTest++/ReportAssert.cpp \
           UnitTest++/Test.cpp \
           UnitTest++/TestDetails.cpp \
           UnitTest++/TestList.cpp \
           UnitTest++/TestReporter.cpp \
           UnitTest++/TestReporterStdout.cpp \
           UnitTest++/TestResults.cpp \
           UnitTest++/TestRunner.cpp \
           UnitTest++/TimeConstraint.cpp \
           UnitTest++/XmlTestReporter.cpp \
           UnitTest++/MultiTactionTestRunner.cpp

win32 {
  HEADERS += UnitTest++/Win32/TimeHelpers.h
  SOURCES += UnitTest++/Win32/TimeHelpers.cpp
  DEFINES += UNITTEST_WIN32_DLL
} else {
  HEADERS += UnitTest++/Posix/TimeHelpers.h
  HEADERS += UnitTest++/Posix/SignalTranslator.h
  SOURCES += UnitTest++/Posix/TimeHelpers.cpp
  SOURCES += UnitTest++/Posix/SignalTranslator.cpp
}
DEFINES += UNITTEST_USE_CUSTOM_STREAMS
