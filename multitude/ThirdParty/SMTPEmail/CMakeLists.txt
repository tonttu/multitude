set(LIBRARY SMTPEmail)

file(GLOB SOURCES src/*.cpp)

cornerstone_add_library(${LIBRARY} SHARED ${SOURCES})

target_compile_definitions(${LIBRARY} PRIVATE SMTP_BUILD)

if(WIN32)
  target_compile_options(${LIBRARY} PRIVATE -wd4251)
endif()

cmake_path(SET dotdot NORMALIZE ${CMAKE_CURRENT_LIST_DIR}/..)

target_include_directories(${LIBRARY} PUBLIC ${dotdot})

target_link_libraries(${LIBRARY}
  PRIVATE Qt5::Core $<$<PLATFORM_ID:Windows>:Qt5::Network>
)

cornerstone_install_lib(${LIBRARY})
