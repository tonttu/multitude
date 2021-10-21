include(../../common.cmake)

set(SOURCES
  Executors.cpp
  LimitedTimeExecutor.cpp
)

add_library(Punctual SHARED ${SOURCES})

target_compile_definitions(Punctual PRIVATE -DPUNCTUAL_EXPORT)

target_link_libraries(Punctual PUBLIC Radiant)
