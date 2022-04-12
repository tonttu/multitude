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
  # For port audio.
  $<$<BOOL:${ENABLE_PORT_AUDIO}>:PortAudioSource.cpp>
  $<$<BOOL:${ENABLE_PORT_AUDIO}>:AudioLoopPortAudio.cpp>
  # For pulse audio.
  $<$<BOOL:${ENABLE_PULSE_AUDIO}>:PulseAudioSource.cpp>
  $<$<BOOL:${ENABLE_PULSE_AUDIO}>:AudioLoopPulseAudio.cpp>
  $<$<BOOL:${ENABLE_PULSE_AUDIO}>:PulseAudioContext.cpp>
)

target_compile_definitions(${LIBRARY}
  PRIVATE
    RESONANT_EXPORT
    $<$<BOOL:${ENABLE_PORT_AUDIO}>:CORNERSTONE_ENABLE_PORT_AUDIO>
    $<$<BOOL:${ENABLE_PULSE_AUDIO}>:CORNERSTONE_ENABLE_PULSE>
)

target_link_libraries(${LIBRARY}
  PRIVATE
    Qt5::Core
    "$<$<BOOL:${ENABLE_PORT_AUDIO}>:portaudio;MultiSndFile>"
    $<$<BOOL:${ENABLE_PULSE_AUDIO}>:pulse>
  PUBLIC
    Radiant
    Valuable
)

cornerstone_install_lib(${LIBRARY})
