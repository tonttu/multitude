set(LIBRARY SMTPEmail)

file(GLOB SOURCES src/*.cpp)

cornerstone_add_library(${LIBRARY} SHARED ${SOURCES})

add_definitions(-DSMTP_BUILD)

if(UNIX)
  # Linux build works just fine with just "Core".
  cornerstone_add_qt_libs(${LIBRARY} PRIVATE "Core")
elseif(WIN32)
  cornerstone_add_qt_libs(${LIBRARY} PRIVATE "Core;Network")
endif()

cmake_path(SET dotdot NORMALIZE ${CMAKE_CURRENT_LIST_DIR}/..)

target_include_directories(${LIBRARY} PUBLIC ${dotdot})

cornerstone_install_lib(${LIBRARY})
