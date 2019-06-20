include(../../common.cmake)

project(Nimble VERSION ${CORNERSTONE_VERSION} DESCRIPTION "A collection of C++ classes for math")

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

add_library(Nimble SHARED ${SOURCES})

find_package(Qt5 COMPONENTS Core PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

include_directories(${Qt5Core_INCLUDE_DIRS})
