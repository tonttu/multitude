include(../../../common.cmake)

add_library(UnitTest++ SHARED)

set(SOURCES
  AssertException.cpp
  Checks.cpp
  CompositeTestReporter.cpp
  CurrentTest.cpp
  DeferredTestReporter.cpp
  DeferredTestResult.cpp
  MemoryOutStream.cpp
  MultiTactionTestRunner.cpp
  ReportAssert.cpp
  RequiredCheckException.cpp
  RequiredCheckTestReporter.cpp
  Test.cpp
  TestDetails.cpp
  TestList.cpp
  TestReporter.cpp
  TestReporterStdout.cpp
  TestResults.cpp
  TestRunner.cpp
  ThrowingTestReporter.cpp
  TimeConstraint.cpp
  XmlTestReporter.cpp
)

if (UNIX)
  list(APPEND SOURCES
    Posix/TimeHelpers.cpp
    Posix/SignalTranslator.cpp
  )
endif()

set_target_properties(UnitTest++ PROPERTIES SOURCES "${SOURCES}")

target_compile_definitions(UnitTest++ PRIVATE
  -DUNITTEST_DLL_EXPORT
  -DUNITTEST_USE_CUSTOM_STREAMS
)

find_package(Qt5 COMPONENTS Xml PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

target_link_libraries(UnitTest++ PRIVATE Qt5::Xml Radiant)
