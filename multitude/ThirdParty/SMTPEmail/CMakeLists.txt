set(LIBRARY SMTPEmail)

file(GLOB SOURCES src/*.cpp)

cornerstone_add_library(${LIBRARY} SHARED ${SOURCES})

add_definitions(-DSMTP_BUILD)

cornerstone_add_qt_libs(${LIBRARY} PRIVATE "Core")

cornerstone_install_lib(${LIBRARY})
