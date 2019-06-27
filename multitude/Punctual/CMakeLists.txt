include(../../common.cmake)

set(SOURCES
  TaskScheduler.cpp
)

add_library(Punctual SHARED ${SOURCES})

target_compile_definitions(Punctual PRIVATE -DPUNCTUAL_EXPORT)

target_link_libraries(Punctual PUBLIC Radiant)
