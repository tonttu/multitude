set(LIBRARY Nimble)

project(${LIBRARY} VERSION ${PROJECT_VERSION_FULL}
  DESCRIPTION "A collection of C++ classes for math"
)

cornerstone_add_library(${LIBRARY} SHARED)

target_sources(${LIBRARY} PRIVATE
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

add_definitions(-DNIMBLE_EXPORT -D_USE_MATH_DEFINES)

cornerstone_add_qt_libs(${LIBRARY} PRIVATE "Core")

target_include_directories(${LIBRARY}
  PUBLIC ${Cornerstone_SOURCE_DIR}/multitude
)

cornerstone_install_lib(${LIBRARY})
