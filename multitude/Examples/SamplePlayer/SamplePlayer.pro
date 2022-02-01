include(../Examples.pri)

SOURCES += Main.cpp

LIBS += $$LIB_RADIANT $$LIB_RESONANT $$LIB_VALUABLE  $$LIB_PATTERNS $$LNK_MULTITUDE

win32 {
	LIBS += -lsndfile
}

unix {
	LIBS += -lsndfile
}