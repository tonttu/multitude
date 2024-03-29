set(LIBRARY UnitTest++)

cornerstone_add_library(${LIBRARY} SHARED)

target_sources(${LIBRARY} PRIVATE
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
  # For Linux
  $<$<PLATFORM_ID:Linux>:Posix/TimeHelpers.cpp>
  $<$<PLATFORM_ID:Linux>:Posix/SignalTranslator.cpp>
  # For Windows
  $<$<PLATFORM_ID:Windows>:Win32/TimeHelpers.cpp>
)

target_compile_options(${LIBRARY} PRIVATE $<$<PLATFORM_ID:Windows>:-wd4251>)

target_compile_definitions(${LIBRARY}
  PRIVATE
    UNITTEST_DLL_EXPORT
  PUBLIC
    UNITTEST_USE_CUSTOM_STREAMS
    $<$<PLATFORM_ID:Windows>:UNITTEST_WIN32_DLL>
    $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
)

target_link_libraries(${LIBRARY} PRIVATE Radiant Qt5::Xml)
target_include_directories(${LIBRARY} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)

cmake_path(SET dotdot NORMALIZE ${CMAKE_CURRENT_LIST_DIR}/..)

# Needs to be system include directory to hide warnings.
target_include_directories(${LIBRARY} SYSTEM PUBLIC ${dotdot})

cornerstone_install_lib(${LIBRARY})
