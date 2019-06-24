include(../../common.cmake)

file(GLOB SOURCES *.cpp)

add_library(Squish STATIC ${SOURCES})

add_definitions(-DUSE_SSE=2)

target_include_directories(Squish PRIVATE .)

set_property(TARGET Squish PROPERTY POSITION_INDEPENDENT_CODE ON)
