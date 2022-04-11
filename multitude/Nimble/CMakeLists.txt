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

target_compile_definitions(${LIBRARY} PRIVATE NIMBLE_EXPORT _USE_MATH_DEFINES)

target_link_libraries(${LIBRARY} PRIVATE RadiantHdr Qt5::Core)

cornerstone_install_lib(${LIBRARY})
