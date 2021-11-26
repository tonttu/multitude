include(../../common.cmake)

set(LIBRARY Nimble)

project(${LIBRARY} VERSION ${CORNERSTONE_VERSION}
  DESCRIPTION "A collection of C++ classes for math"
)

set(SOURCES
  ClipStack.cpp
  Circle.cpp
  KeyStone.cpp
  LensCorrection.cpp
  Path.cpp
  Plane.cpp
  Random.cpp
  Rectangle.cpp
  Splines.cpp
)

cornerstone_add_library(${LIBRARY} SHARED ${SOURCES})

add_definitions(-DNIMBLE_EXPORT -D_USE_MATH_DEFINES)

find_package(Qt5 COMPONENTS Core PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

include_directories(${Qt5Core_INCLUDE_DIRS})
