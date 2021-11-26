include(../../common.cmake)

set(LIBRARY Punctual)

set(SOURCES
  Executors.cpp
  LimitedTimeExecutor.cpp
)

cornerstone_add_library(${LIBRARY} SHARED ${SOURCES})

target_compile_definitions(${LIBRARY} PRIVATE -DPUNCTUAL_EXPORT)

target_link_libraries(${LIBRARY} PUBLIC Radiant)

cornerstone_install_lib(${LIBRARY})
