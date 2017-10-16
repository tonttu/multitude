include(../../../Applications/Applications.pri)

SOURCES += Main.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE
LIBS += $$LIB_PATTERNS $$LIB_VALUABLE $$LIB_V8

QT += core network xml

unix: PKGCONFIG += portaudio-2.0

win* {
  INCLUDEPATH += $$CORNERSTONE_DEPS_PATH/portaudio/include
  LIBS += -L$$CORNERSTONE_DEPS_PATH/portaudio/lib -lportaudio_x64

  INCLUDEPATH += $$CORNERSTONE_DEPS_PATH/libsndfile/include
  LIBS += -L$$CORNERSTONE_DEPS_PATH/libsndfile/lib -llibsndfile-1

  LIBS += -lole32 -luser32
}

include(../../../Applications/Applications_end.pri)
