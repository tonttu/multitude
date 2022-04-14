set(LIBRARY Punctual)

cornerstone_add_library(${LIBRARY} SHARED)

target_sources(${LIBRARY} PRIVATE
  Executors.cpp
  LimitedTimeExecutor.cpp
)

target_compile_definitions(${LIBRARY} PRIVATE -DPUNCTUAL_EXPORT)

target_link_libraries(${LIBRARY} PRIVATE Qt5::Core PUBLIC Radiant MultiFolly)

cornerstone_install_lib(${LIBRARY})
