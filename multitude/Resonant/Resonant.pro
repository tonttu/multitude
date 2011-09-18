include(../multitude.pri)

HEADERS += Application.hpp
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
HEADERS += ModulePulseAudio.hpp
HEADERS += ModuleRectPanner.hpp
HEADERS += ModuleSamplePlayer.hpp
HEADERS += PulseAudioCore.hpp
HEADERS += Resonant.hpp
HEADERS += SoundRectangle.hpp

SOURCES += Application.cpp
SOURCES += AudioFileHandler.cpp
SOURCES += AudioLoop.cpp
SOURCES += DSPNetwork.cpp
SOURCES += Module.cpp
SOURCES += ModuleFilePlay.cpp
SOURCES += ModuleGain.cpp
SOURCES += ModuleOutCollect.cpp
SOURCES += ModulePanner.cpp
SOURCES += ModulePulseAudio.cpp
SOURCES += ModuleRectPanner.cpp
SOURCES += ModuleSamplePlayer.cpp
SOURCES += PulseAudioCore.cpp
SOURCES += SoundRectangle.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE $$LIB_PATTERNS $$LIB_VALUABLE

linux-*:LIBS += -lpulse
unix:LIBS += -lportaudio -lsndfile

include(../library.pri)

win32 {
  DEFINES += RESONANT_EXPORT
  win64 {
    QMAKE_LIBDIR += $$DDK_PATH\\lib\\win7\\amd64
    INCLUDEPATH += ..\\Win64x\\include\\portaudio
    LIBS += -llibsndfile-1 -lportaudio -lOle32 -lUser32
  }
  else {
    QMAKE_LIBDIR += $$DDK_PATH\\lib\\win7\\i386
    INCLUDEPATH += ..\\Win32x\\include\\portaudio ..\\Win32x\\include\\libsndfile
    LIBS += -llibsndfile-1 -lportaudio_x86
  }
}
