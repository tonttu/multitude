include(../../common.cmake)

project(Resonant VERSION ${CORNERSTONE_VERSION} DESCRIPTION "A collection of C++ classes for sound processing.")

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

  target_link_libraries(Resonant portaudio)
endif()

if(ENABLE_PULSE_AUDIO)
  add_definitions(-DCORNERSTONE_ENABLE_PULSE)

  list(APPEND SOURCES
    PulseAudioSource.cpp
    AudioLoopPulseAudio.cpp
    PulseAudioContext.cpp
  )

  target_link_libraries(Resonant pulse)
endif()

set_target_properties(Resonant PROPERTIES SOURCES "${SOURCES}")

add_definitions(-DRESONANT_EXPORT)

if(UNIX)
  target_link_libraries(Resonant sndfile)
endif()

find_package(Qt5 COMPONENTS Core PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Gui PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

target_link_libraries(Resonant Qt5::Core)

target_include_directories(Resonant PUBLIC ${Qt5Gui_INCLUDE_DIRS})
