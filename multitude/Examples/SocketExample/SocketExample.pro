include(../Examples.pri)

SOURCES += Main.cpp

LIBS += $$LIB_RADIANT  $$LIB_PATTERNS

win32 {
	CONFIG += console
}