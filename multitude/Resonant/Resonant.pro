include(../../cornerstone.pri)

unix: PKGCONFIG += sndfile portaudio-2.0

HEADERS += AudioFileHandler.hpp \
    LimiterAlgorithm.hpp \
    ModuleBufferPlayer.hpp \
    PortAudioSource.hpp \
    AudioLoopPortAudio.hpp \
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
    ModuleBufferPlayer.cpp \
    PortAudioSource.cpp \
    AudioLoopPortAudio.cpp
SOURCES += DSPNetwork.cpp
SOURCES += Module.cpp
SOURCES += ModuleFilePlay.cpp
SOURCES += ModuleGain.cpp
SOURCES += ModuleOutCollect.cpp
SOURCES += ModulePanner.cpp
SOURCES += ModuleRectPanner.cpp
SOURCES += ModuleSamplePlayer.cpp
SOURCES += SoundRectangle.cpp

enable-pulse {
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

include(../library.pri)

DEFINES += RESONANT_EXPORT

win* {
  QMAKE_LIBDIR += $$DDK_PATH\\lib\\win7\\amd64
  INCLUDEPATH += ..\\Win64x\\include\\portaudio
  LIBS += -llibsndfile-1 -lportaudio_x64 -lOle32 -lUser32
}
