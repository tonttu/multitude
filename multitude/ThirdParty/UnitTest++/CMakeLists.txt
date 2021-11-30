include(../../../common.cmake)

set(LIBRARY UnitTest++)

cornerstone_add_library(${LIBRARY} SHARED)

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
      -DUNITTEST_WIN32_DLL -D_CRT_SECURE_NO_WARNINGS
  )

  list(APPEND SOURCES
    Win32/TimeHelpers.cpp
  )
endif()

set_target_properties(${LIBRARY} PROPERTIES SOURCES "${SOURCES}")

target_compile_definitions(${LIBRARY}
  PRIVATE -DUNITTEST_DLL_EXPORT
  PUBLIC -DUNITTEST_USE_CUSTOM_STREAMS
)

target_link_libraries(${LIBRARY} PRIVATE Radiant)

add_qt_libs(${QT_PATH} ${LIBRARY} PRIVATE "Xml")

cornerstone_install_lib(${LIBRARY})
