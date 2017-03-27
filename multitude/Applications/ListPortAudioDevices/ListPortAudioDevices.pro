include(../Applications.pri)

TARGET = ListPortAudioDevices$${CORNERSTONE_APP_SUFFIX}

SOURCES += Main.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE
LIBS += $$LIB_PATTERNS $$LIB_VALUABLE $$LIB_V8

QT += core network xml

unix: PKGCONFIG += portaudio-2.0

win* {
	INCLUDEPATH += ../../Win64x/include/portaudio
	INCLUDEPATH += ../../Win64x/include/libsndfile

  QMAKE_LIBDIR += $$DDK_PATH\\lib\\win7\\amd64
  LIBS += -llibsndfile-1 -lportaudio_x64 -lole32 -luser32
}

include(../Applications_end.pri)
