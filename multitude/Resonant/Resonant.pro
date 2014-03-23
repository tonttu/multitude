include(../multitude.pri)

unix: PKGCONFIG += sndfile portaudio-2.0

HEADERS += AudioFileHandler.hpp
HEADERS += AudioLoop.hpp
HEADERS += AudioLoop_private.hpp
HEADERS += DSPNetwork.hpp
HEADERS += Export.hpp
HEADERS += ModuleFilePlay.hpp
HEADERS += ModuleGain.hpp
HEADERS += Module.hpp
HEADERS += ModuleOutCollect.hpp
HEADERS += ModulePanner.hpp
!enable-taction:HEADERS += ModulePulseAudio.hpp
HEADERS += ModuleRectPanner.hpp
HEADERS += ModuleSamplePlayer.hpp
!enable-taction:HEADERS += PulseAudioCore.hpp
HEADERS += Resonant.hpp
HEADERS += SoundRectangle.hpp

SOURCES += AudioFileHandler.cpp
SOURCES += AudioLoop.cpp
SOURCES += DSPNetwork.cpp
SOURCES += Module.cpp
SOURCES += ModuleFilePlay.cpp
SOURCES += ModuleGain.cpp
SOURCES += ModuleOutCollect.cpp
SOURCES += ModulePanner.cpp
!enable-taction:SOURCES += ModulePulseAudio.cpp
SOURCES += ModuleRectPanner.cpp
SOURCES += ModuleSamplePlayer.cpp
!enable-taction:SOURCES += PulseAudioCore.cpp
SOURCES += SoundRectangle.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE $$LIB_PATTERNS $$LIB_VALUABLE

!enable-taction:linux-*:LIBS += -lpulse

include(../library.pri)

DEFINES += RESONANT_EXPORT

win* {
  QMAKE_LIBDIR += $$DDK_PATH\\lib\\win7\\amd64
  INCLUDEPATH += ..\\Win64x\\include\\portaudio
  LIBS += -llibsndfile-1 -lportaudio_x64 -lOle32 -lUser32
}
