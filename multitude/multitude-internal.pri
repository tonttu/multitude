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


linux-g++ {
  TARGET_SYSTEM = linux-$$QMAKE_HOST.arch
}

exists($$DEPS_DIR) {
  message("Adding extra deps dir")
  INCLUDEPATH += $$DEPS_DIR/$$TARGET_SYSTEM/include
  DEPS_LIBDIR = $$DEPS_DIR/$${TARGET_SYSTEM}lib
  LIBS += -L$$DEPS_LIBDIR
  PKG_CONFIG_PATH = $$DEPS_LIBDIR/pkgconfig
  message("PKG_CONFIG_PATH="$$PKG_CONFIG_PATH)
  !exists($$PKG_CONFIG_PATH) {
    error("No PKG_CONFIG_PATH dir")
  }
}

!exists($$DEPS_DIR) {
  error("No deps dir, please define DEPS_DIR");
}

unix:LIBS += -L$$ROOT/lib/$$TARGET_SYSTEM

VERSION = 2.7

LIB_NIMBLE = -lNimble
LIB_RADIANT = -lRadiant
LIB_VALUABLE = -lValuable
