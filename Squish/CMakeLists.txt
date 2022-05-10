set(LIBRARY Squish)

file(GLOB SOURCES *.cpp)

cornerstone_add_library(${LIBRARY} STATIC ${SOURCES})

target_compile_definitions(${LIBRARY} PRIVATE USE_SSE=2)

target_include_directories(${LIBRARY} PRIVATE .)

set_property(TARGET ${LIBRARY} PROPERTY POSITION_INDEPENDENT_CODE ON)

cornerstone_install_lib(${LIBRARY})
