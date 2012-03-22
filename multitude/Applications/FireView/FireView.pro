include(../Applications.pri)

HEADERS += CamView.hpp \ 
    Binning.hpp
HEADERS += MainWindow.hpp 
HEADERS += ParamView.hpp

SOURCES += CamView.cpp \ 
    Binning.cpp
SOURCES += MainWindow.cpp 
SOURCES += ParamView.cpp 
SOURCES += Main.cpp

LIBS += $$LIB_RADIANT $$LIB_LUMINOUS $$LIB_VALUABLE $$LIB_NIMBLE
LIBS += $$LIB_PATTERNS $$LIB_V8

CONFIG += qt

QT = core gui opengl xml

LIBS += $$LIB_GLU

include(../Applications_end.pri)
