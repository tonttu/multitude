include(../multitude.pri)

HEADERS += Export.hpp
HEADERS += VideoFFMPEG.hpp

SOURCES += VideoFFMPEG.cpp

LIBS += $$MULTI_FFMPEG_LIBS

LIBS += $$LIB_RADIANT $$LIB_PATTERNS

win32 {
	DEFINES += SCREENPLAY_EXPORT	
	
	win64:INCLUDEPATH += ../Win64x/include/ffmpeg
	else:INCLUDEPATH += ../Win32x/include/ffmpeg
}


include(../library.pri)
