set(BINARY ListPortAudioDevices)
add_executable(${BINARY} Main.cpp)

if(UNIX)
  target_link_libraries(${BINARY} PRIVATE PkgConfig::PortAudio)
elseif(WIN32)
  target_link_libraries(${BINARY} PRIVATE portaudio)
endif()

cornerstone_install_bin(${BINARY})
