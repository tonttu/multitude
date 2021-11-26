include(../../common.cmake)

set(LIBRARY Resonant)

cornerstone_add_library(${LIBRARY} SHARED)

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

  target_link_libraries(${LIBRARY} PRIVATE portaudio)
endif()

if(ENABLE_PULSE_AUDIO)
  add_definitions(-DCORNERSTONE_ENABLE_PULSE)

  list(APPEND SOURCES
    PulseAudioSource.cpp
    AudioLoopPulseAudio.cpp
    PulseAudioContext.cpp
  )

  target_link_libraries(${LIBRARY} PRIVATE pulse)
endif()

set_target_properties(${LIBRARY} PROPERTIES SOURCES "${SOURCES}")

target_compile_definitions(${LIBRARY} PRIVATE -DRESONANT_EXPORT)

if(WIN32)
  find_package(SndFile CONFIG REQUIRED)
  target_link_libraries(${LIBRARY} PRIVATE SndFile::sndfile)

  find_package(portaudio CONFIG REQUIRED)
  target_link_libraries(${LIBRARY} PRIVATE portaudio)
endif()

target_link_libraries(${LIBRARY} PUBLIC Radiant Valuable)
