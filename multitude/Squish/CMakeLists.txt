set(LIBRARY Squish)

file(GLOB SOURCES *.cpp)

cornerstone_add_library(${LIBRARY} STATIC ${SOURCES})

add_definitions(-DUSE_SSE=2)

target_include_directories(${LIBRARY} PRIVATE .)

set_property(TARGET ${LIBRARY} PROPERTY POSITION_INDEPENDENT_CODE ON)

cornerstone_install_lib(${LIBRARY})
