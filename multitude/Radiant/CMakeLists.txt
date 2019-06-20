include(../../common.cmake)

project(Radiant VERSION ${CORNERSTONE_VERSION} DESCRIPTION "A collection of C++ utility classes")

set(SOURCES
  Mime.cpp
  ThreadChecks.cpp
  TraceDuplicateFilter.cpp
  TraceStdFilter.cpp
  TraceSeverityFilter.cpp
  TraceQIODeviceFilter.cpp
  TemporaryDir.cpp
  ObjectPool.cpp
  CommandLineArguments.cpp
  SynchronizedMultiQueue.cpp
  ProcessRunner.cpp
)

add_library(Radiant SHARED ${SOURCES})

find_package(Qt5 COMPONENTS Core REQUIRED)

target_link_libraries(Radiant Qt5::Core)
