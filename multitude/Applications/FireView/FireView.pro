include(../Applications.pri)

HEADERS += CamView.hpp 
HEADERS += MainWindow.hpp 
HEADERS += ParamView.hpp

SOURCES += CamView.cpp 
SOURCES += MainWindow.cpp 
SOURCES += ParamView.cpp 
SOURCES += Main.cpp

LIBS += $$LIB_RADIANT $$LIB_LUMINOUS $$LIB_VALUABLE $$LIB_NIMBLE
LIBS += $$LIB_PATTERNS $$LIB_V8

CONFIG += qt

QT = core gui opengl xml

linux-*:LIBS += -lGLU
win32:LIBS += -lGLU32

include(../Applications_end.pri)
