include(../../../common.cmake)

set(BINARY ListPortAudioDevices)
add_executable(${BINARY} Main.cpp)

if(UNIX)
  pkg_check_modules(PortAudio REQUIRED IMPORTED_TARGET portaudio-2.0)

  target_link_libraries(${BINARY} PRIVATE PkgConfig::PortAudio)
elseif(WIN32)
  find_package(portaudio CONFIG REQUIRED)
  target_link_libraries(${BINARY} PRIVATE portaudio)
endif()

find_package(Qt5 COMPONENTS Core PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

target_link_libraries(${BINARY} PRIVATE Qt5::Core)
