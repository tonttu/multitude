# Expected is header only library.

set (LIBRARY OurExpected)

add_library(${LIBRARY} INTERFACE)

target_include_directories(${LIBRARY} INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/include
)
