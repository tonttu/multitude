include(../multitude.pri)

HEADERS += Export.hpp
HEADERS += VideoFFMPEG.hpp
HEADERS += ScreenPlay.hpp

SOURCES += VideoFFMPEG.cpp

LIBS += $$MULTI_FFMPEG_LIBS

LIBS += $$LIB_RADIANT $$LIB_PATTERNS $$LIB_NIMBLE

DEFINES += SCREENPLAY_EXPORT	

include(../library.pri)
