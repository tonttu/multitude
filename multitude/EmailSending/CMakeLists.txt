include(../../common.cmake)

set(LIBRARY EmailSending)

project(${LIBRARY}
  VERSION ${CORNERSTONE_VERSION}
  DESCRIPTION "Email sending library"
)

set(SOURCES
  Email.cpp
  Sender.cpp
  SendImplementation.cpp
)

cornerstone_add_library(${LIBRARY} SHARED ${SOURCES})

target_compile_definitions(${LIBRARY} PRIVATE -DEMAIL_EXPORT)

target_link_libraries(${LIBRARY}
  PUBLIC Valuable
  PRIVATE Radiant SMTPEmail
)

target_include_directories(${LIBRARY} PUBLIC ${THIRDPARTY_PATH})

add_qt_libs(${QT_PATH} ${LIBRARY} PRIVATE "Network")

cornerstone_install_lib(${LIBRARY})
