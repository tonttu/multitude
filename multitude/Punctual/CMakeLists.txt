include(../../common.cmake)

project(Punctual VERSION ${CORNERSTONE_VERSION} DESCRIPTION "Folly executors for Cornerstone")

set(SOURCES
  TaskScheduler.cpp
)

add_library(Punctual SHARED ${SOURCES})

add_definitions(-DPUNCTUAL_EXPORT)

target_link_libraries(Punctual Radiant)
