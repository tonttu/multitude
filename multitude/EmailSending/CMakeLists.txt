include(../../common.cmake)

project(EmailSending VERSION ${CORNERSTONE_VERSION} DESCRIPTION "Email sending library")

set(SOURCES
  Email.cpp
  Sender.cpp
  SendImplementation.cpp
)

add_library(EmailSending SHARED ${SOURCES})

add_definitions(-DEMAIL_EXPORT)

target_link_libraries(EmailSending Valuable Radiant)

target_include_directories(EmailSending PUBLIC ${THIRDPARTY_PATH})
