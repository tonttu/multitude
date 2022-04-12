set(LIBRARY Resonant)

cornerstone_add_library(${LIBRARY} SHARED)

target_sources(${LIBRARY} PRIVATE
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
  target_sources(${LIBRARY} PRIVATE
    PortAudioSource.cpp
    AudioLoopPortAudio.cpp
  )

  if(UNIX)
    target_link_libraries(${LIBRARY} PRIVATE sndfile)
  endif()

  target_link_libraries(${LIBRARY} PRIVATE portaudio)
endif()

if(ENABLE_PULSE_AUDIO)
  target_sources(${LIBRARY} PRIVATE
    PulseAudioSource.cpp
    AudioLoopPulseAudio.cpp
    PulseAudioContext.cpp
  )

  target_link_libraries(${LIBRARY} PRIVATE pulse)
endif()

target_compile_definitions(${LIBRARY}
  PRIVATE
    RESONANT_EXPORT
    $<$<BOOL:${ENABLE_PORT_AUDIO}>:CORNERSTONE_ENABLE_PORT_AUDIO>
    $<$<BOOL:${ENABLE_PULSE}>:CORNERSTONE_ENABLE_PULSE>
)

if(WIN32)
  target_link_libraries(${LIBRARY} PRIVATE SndFile::sndfile portaudio)
endif()

target_link_libraries(${LIBRARY} PRIVATE Qt5::Core PUBLIC Radiant Valuable)

cornerstone_install_lib(${LIBRARY})
