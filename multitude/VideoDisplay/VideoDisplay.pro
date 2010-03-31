include(../multitude.pri)

HEADERS += AudioTransfer.hpp
HEADERS += Export.hpp
HEADERS += ShowGL.hpp
HEADERS += SubTitles.hpp
HEADERS += VideoIn.hpp
HEADERS += VideoInFFMPEG.hpp

SOURCES += VideoIn.cpp
SOURCES += VideoInFFMPEG.cpp

SOURCES += AudioTransfer.cpp
SOURCES += ShowGL.cpp
SOURCES += SubTitles.cpp

unix:PKGCONFIG += libavutil

DEFINES += __STDC_CONSTANT_MACROS

LIBS += $$LIB_RESONANT $$LIB_SCREENPLAY $$LIB_LUMINOUS $$LIB_NIMBLE
LIBS += $$LIB_RADIANT $$LIB_POETIC $$LIB_OPENGL $$LIB_RESONANT
LIBS += $$LIB_PATTERNS $$LIB_VALUABLE $$LIB_GLEW

macx:LIBS += -framework,OpenGL

include(../library.pri)

win32 {
    DEFINES += VIDEODISPLAY_EXPORT
}
