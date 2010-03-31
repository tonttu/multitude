include(../Applications.pri)

HEADERS += VideoWindow.hpp

SOURCES += VideoWindow.cpp
SOURCES += Main.cpp

LIBS += $$LIB_VIDEODISPLAY $$LIB_POETIC $$LIB_RESONANT
LIBS += $$LIB_SCREENPLAY $$LIB_LUMINOUS  $$LIB_PATTERNS
LIBS += $$LIB_VALUABLE $$LIB_RADIANT $$LIB_NIMBLE
LIBS += $$MULTI_FFMPEG_LIBS

CONFIG += qt 

QT = core gui opengl xml

include(../Applications_end.pri)
