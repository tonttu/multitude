include(../../../Applications/Applications.pri)

SOURCES += Main.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE
LIBS += $$LIB_PATTERNS $$LIB_VALUABLE

QT += core network xml

unix: PKGCONFIG += portaudio-2.0

win*: LIBS += -lportaudio -lsndfile

include(../../../Applications/Applications_end.pri)
