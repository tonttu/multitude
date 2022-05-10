set(LIBRARY EmailSending)

project(${LIBRARY}
  VERSION ${PROJECT_VERSION_FULL}
  DESCRIPTION "Email sending library"
)

cornerstone_add_library(${LIBRARY} SHARED)

target_sources(${LIBRARY} PRIVATE
  Email.cpp
  Sender.cpp
  SendImplementation.cpp
)

target_compile_definitions(${LIBRARY} PRIVATE -DEMAIL_EXPORT)

target_link_libraries(${LIBRARY}
  PRIVATE Radiant SMTPEmail Qt5::Network
  PUBLIC Valuable
)

cornerstone_install_lib(${LIBRARY})
