include(../../../common.cmake)

set(LIBRARY SMTPEmail)

file(GLOB SOURCES src/*.cpp)

cornerstone_add_library(${LIBRARY} SHARED ${SOURCES})

add_definitions(-DSMTP_BUILD)

add_qt_libs(${QT_PATH} ${LIBRARY} PRIVATE "Core")

cornerstone_install_lib(${LIBRARY})
