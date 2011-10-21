include(../Applications.pri)

SOURCES += Main.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE
LIBS += $$LIB_PATTERNS $$LIB_LUMINOUS $$LIB_VALUABLE $$LIB_V8

include(../Applications_end.pri)
