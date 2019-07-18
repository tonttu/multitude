include(../../common.cmake)

add_library(Resonant SHARED)

set(SOURCES
  AudioFileHandler.cpp
  LimiterAlgorithm.cpp
  ModuleBufferPlayer.cpp
  DSPNetwork.cpp
  Module.cpp
  ModuleFilePlay.cpp
  ModuleGain.cpp
  ModuleOutCollect.cpp
  ModulePanner.cpp
  ModuleSamplePlayer.cpp
  SoundRectangle.cpp
)

if(ENABLE_PORT_AUDIO)
  add_definitions(-DCORNERSTONE_ENABLE_PORT_AUDIO)

  list(APPEND SOURCES
    PortAudioSource.cpp
    AudioLoopPortAudio.cpp
  )

  target_link_libraries(Resonant PRIVATE portaudio)
endif()

if(ENABLE_PULSE_AUDIO)
  add_definitions(-DCORNERSTONE_ENABLE_PULSE)

  list(APPEND SOURCES
    PulseAudioSource.cpp
    AudioLoopPulseAudio.cpp
    PulseAudioContext.cpp
  )

  target_link_libraries(Resonant PRIVATE pulse)
endif()

set_target_properties(Resonant PROPERTIES SOURCES "${SOURCES}")

target_compile_definitions(Resonant PRIVATE -DRESONANT_EXPORT)

if(UNIX)
  target_include_libraries(Resonant PRIVATE ${VCPKG_ROOT}/include)
elseif(WIN32)
  target_link_libraries(Resonant PRIVATE ${VCPKG_ROOT}/lib/libsndfile-1.lib)
  target_link_libraries(Resonant PRIVATE ${VCPKG_ROOT}/lib/libsndfile-1.lib)
endif()

target_link_libraries(Resonant PUBLIC Radiant Valuable)
