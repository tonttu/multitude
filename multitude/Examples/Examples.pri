include(../multitude.pri)

INCLUDEPATH += ../../
DEPENDPATH += ../../

macx:LIBS += -framework,Cocoa

win32:CONFIG += console
