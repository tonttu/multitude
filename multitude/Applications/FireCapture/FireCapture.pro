include(../Applications.pri)

SOURCES += FireCapture.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE
LIBS += $$LIB_PATTERNS $$LIB_LUMINOUS $$LIB_VALUABLE $$LIB_V8 $$LIB_OPENGL

include(../Applications_end.pri)
