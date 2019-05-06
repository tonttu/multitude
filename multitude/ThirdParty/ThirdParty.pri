# Don't use default multitude install rules because we want these files to go
# under ThirdParty. We override this at the end of this file.
skip_install_target = true
include(../setup-lib.pri)

# Hack required to build libQxtNetwork
SHARED_LIB_SUFFIX=
win32:CONFIG(debug,debug|release) {
  SHARED_LIB_SUFFIX=_d
}

contains(LIBS, -lQxtCore$${SHARED_LIB_SUFFIX}) {
  LIBS -= -lQxtCore$${SHARED_LIB_SUFFIX}
  LIBS += -lQxtCore$${CORNERSTONE_LIB_SUFFIX}
}

# Create custom install target for headers in release mode
CONFIG(release, debug|release) {
  enable-sdk {
    $$installFiles(/include/ThirdParty/$$TARGET_WITHOUT_VERSION, EXPORT_HEADERS)
  }
}
