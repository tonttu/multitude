# rgbeasy is header only library.

set (LIBRARY RGBEasy)

add_library(${LIBRARY} INTERFACE)

cmake_path(SET include NORMALIZE ${CMAKE_CURRENT_LIST_DIR}/include)

target_include_directories(${LIBRARY} INTERFACE ${include})
