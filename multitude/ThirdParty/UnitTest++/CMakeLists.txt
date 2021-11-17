include(../../../common.cmake)

set(LIBRARY UnitTest++)

add_library(${LIBRARY} SHARED)

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

if(WIN32)
  target_compile_definitions(${LIBRARY} PUBLIC
      -DUNITTEST_WIN32_DLL -D_CRT_SECURE_NO_WARNINGS -DUNITTEST_USE_CUSTOM_STREAMS
  )

  list(APPEND SOURCES
    Win32/TimeHelpers.cpp
  )
endif()

set_target_properties(${LIBRARY} PROPERTIES SOURCES "${SOURCES}")

target_compile_definitions(${LIBRARY} PRIVATE
  -DUNITTEST_DLL_EXPORT
)

find_package(Qt5 COMPONENTS Xml PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

target_link_libraries(${LIBRARY} PRIVATE Qt5::Xml Radiant)
