include(../multitude.pri)

HEADERS += Application.hpp
HEADERS += AudioFileHandler.hpp
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
SOURCES += ModuleRectPanner.cpp
SOURCES += ModuleSamplePlayer.cpp
SOURCES += SoundRectangle.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE $$LIB_PATTERNS $$LIB_VALUABLE


unix: PKGCONFIG += portaudio-2.0 sndfile

include(../library.pri)

win32 {
  DEFINES += RESONANT_EXPORT
  win64 {
    INCLUDEPATH += ..\Win64x\include\portaudio
    LIBS += -llibsndfile-1 -lportaudio -lOle32 -lUser32
  }
  else {
    INCLUDEPATH += ..\Win32x\include\portaudio ..\Win32x\include\libsndfile
    LIBS += -llibsndfile-1 -lportaudio_x86
  }
}
