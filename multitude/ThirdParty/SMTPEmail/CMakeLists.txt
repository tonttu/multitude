include(../../../common.cmake)

file(GLOB SOURCES src/*.cpp)

add_library(SMTPEmail SHARED ${SOURCES})

add_definitions(-DSMTP_BUILD)

find_package(Qt5 COMPONENTS Core PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Network PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

target_link_libraries(SMTPEmail Qt5::Core Qt5::Network)
