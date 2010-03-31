include(../multitude.pri)

INCLUDEPATH += ../../
DEPENDPATH += ../../

LIBS += $${MULTI_LIB_FLAG}../../lib

macx:LIBS += -framework,Cocoa

win32:CONFIG += console