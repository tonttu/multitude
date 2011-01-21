include(../Examples.pri)

SOURCES += CSVLoad.cpp

LIBS += $$LIB_RADIANT  $$LIB_PATTERNS $$LIB_VALUABLE

win32 {
	CONFIG += console
}
