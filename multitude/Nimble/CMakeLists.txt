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

add_qt_libs(${QT_PATH} ${LIBRARY} PRIVATE "Core")

cornerstone_install_lib(${LIBRARY})
