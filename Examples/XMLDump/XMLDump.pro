include(../Examples.pri)

SOURCES += Main.cpp

LIBS += $$LIB_RADIANT $$LIB_VALUABLE $$LIB_PATTERNS -lSDLmain

win32 {
    CONFIG += console
}
