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
  LIBS += -L$$DEPS_DIR/$$TARGET_SYSTEM/lib
}

!exists($$DEPS_DIR) {
  message("No deps dir");
}

unix:LIBS += -L$$ROOT/lib/$$TARGET_SYSTEM

VERSION = 2.7
