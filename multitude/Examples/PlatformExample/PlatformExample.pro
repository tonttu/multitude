include(../Examples.pri)

win32: CONFIG += console

SOURCES += Main.cpp

LIBS += $$LIB_RADIANT  $$LIB_PATTERNS

