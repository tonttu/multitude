include(../Examples.pri)

SOURCES += UDPExample.cpp

LIBS += $$LIB_RADIANT  $$LIB_PATTERNS

win32 {
    CONFIG += console
}
