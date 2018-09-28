include(../../../Applications/Applications.pri)

SOURCES += Main.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE
LIBS += $$LIB_PATTERNS $$LIB_VALUABLE $$LIB_V8

QT += core network xml

unix: PKGCONFIG += portaudio-2.0

win*: LIBS += -lportaudio -llibsndfile-1

include(../../../Applications/Applications_end.pri)
