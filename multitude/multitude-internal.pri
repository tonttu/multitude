ROOT = $$PWD

INCLUDEPATH += $$ROOT $$ROOT/ThirdParty

macx {
  QMAKE_CXX=/usr/local/bin/ccache $$QMAKE_CXX
  QMAKE_CC=/usr/local/bin/ccache $$QMAKE_CC
}

linux* {
#  LIBS += -L/usr/lib/x86_64-linux-gnu/
  QMAKE_CXX=ccache $$QMAKE_CXX
  QMAKE_CC=ccache $$QMAKE_CC
}

TARGET_SYSTEM=$$(TARGET_SYSTEM)

linux-g++ {
  TARGET_SYSTEM = linux-$$QMAKE_HOST.arch
}

android {
  CONFIG += mobile
  DEFINES += RADIANT_MOBILE
  message("QMAKE_HOST="$$QMAKE_HOST)
}

exists($$DEPS_DIR) {
  message("Adding extra deps dir")
  INCLUDEPATH += $$DEPS_DIR/$$TARGET_SYSTEM/include
  DEPS_LIBDIR = $$DEPS_DIR/$${TARGET_SYSTEM}/lib
  LIBS += -L$$DEPS_LIBDIR
}

!exists($$DEPS_DIR) {
  error("No deps dir, please define DEPS_DIR");
}

MULTI_LIBDIR=$$ROOT/lib/$$TARGET_SYSTEM
unix:LIBS += -L$$MULTI_LIBDIR

VERSION = 2.7

LIB_NIMBLE = -lNimble
LIB_RADIANT = -lRadiant
LIB_VALUABLE = -lValuable
