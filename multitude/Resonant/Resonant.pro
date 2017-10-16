include(../../cornerstone.pri)

unix: PKGCONFIG += sndfile

HEADERS += AudioFileHandler.hpp \
    LimiterAlgorithm.hpp \
    ModuleBufferPlayer.hpp \
    SourceInfo.hpp
HEADERS += AudioLoop.hpp
HEADERS += DSPNetwork.hpp
HEADERS += Export.hpp
HEADERS += ModuleFilePlay.hpp
HEADERS += ModuleGain.hpp
HEADERS += Module.hpp
HEADERS += ModuleOutCollect.hpp
HEADERS += ModulePanner.hpp
HEADERS += ModuleRectPanner.hpp
HEADERS += ModuleSamplePlayer.hpp
HEADERS += Resonant.hpp
HEADERS += SoundRectangle.hpp

SOURCES += AudioFileHandler.cpp \
    LimiterAlgorithm.cpp \
    ModuleBufferPlayer.cpp
SOURCES += DSPNetwork.cpp
SOURCES += Module.cpp
SOURCES += ModuleFilePlay.cpp
SOURCES += ModuleGain.cpp
SOURCES += ModuleOutCollect.cpp
SOURCES += ModulePanner.cpp
SOURCES += ModuleRectPanner.cpp
SOURCES += ModuleSamplePlayer.cpp
SOURCES += SoundRectangle.cpp

enable-port-audio {
  DEFINES += CORNERSTONE_ENABLE_PORT_AUDIO

  HEADERS += PortAudioSource.hpp
  HEADERS += AudioLoopPortAudio.hpp

  SOURCES += PortAudioSource.cpp
  SOURCES += AudioLoopPortAudio.cpp

  unix: PKGCONFIG += portaudio-2.0

  win32 {
    INCLUDEPATH += $$CORNERSTONE_DEPS_PATH/portaudio/include
    LIBS += -L$$CORNERSTONE_DEPS_PATH/portaudio/lib -lportaudio_x64
  }
}

enable-pulse-audio {
  DEFINES += CORNERSTONE_ENABLE_PULSE

  HEADERS += PulseAudioSource.hpp
  HEADERS += AudioLoopPulseAudio.hpp
  HEADERS += PulseAudioContext.hpp
  SOURCES += PulseAudioSource.cpp
  SOURCES += AudioLoopPulseAudio.cpp
  SOURCES += PulseAudioContext.cpp

  LIBS += -lpulse
}

LIBS += $$LIB_RADIANT $$LIB_NIMBLE $$LIB_PATTERNS $$LIB_VALUABLE

include(../../library.pri)

DEFINES += RESONANT_EXPORT

win* {
  INCLUDEPATH += $$CORNERSTONE_DEPS_PATH/libsndfile/include
  LIBS += -L$$CORNERSTONE_DEPS_PATH/libsndfile/lib -llibsndfile-1

  LIBS += -lOle32 -lUser32
}
