# Don't use default multitude install rules because we want these files to go
# under ThirdParty. We override this at the end of this file.
include(../../cornerstone.pri)

SHARED_LIB_SUFFIX=
win32:CONFIG(debug,debug|release) {
  SHARED_LIB_SUFFIX=_d
}

skip_multitude_install_targets = true
include(../library.pri)

# Create install targets for source code and headers in release mode
CONFIG(release, debug|release) {
  $$installFiles(/include/ThirdParty/$$TARGET_WITHOUT_VERSION, EXPORT_HEADERS)
  $$installFiles(/src/multitude/ThirdParty/$$TARGET_WITHOUT_VERSION, ALL_SOURCE_CODE)
}

# Required to build libqxt
contains(LIBS, -lQxtCore$${SHARED_LIB_SUFFIX}) {
  LIBS -= -lQxtCore$${SHARED_LIB_SUFFIX}
  LIBS += -lQxtCore$${CORNERSTONE_LIB_SUFFIX}
}

# Force override DESTDIR (required by libqxt)
DESTDIR = $$shadowed($$PWD)/../lib
# Since we changed DESTDIR, we must update DLL installation source
win32 {
  dlls.files = $${DESTDIR}/$${TARGET}.dll

  # There doesn't seem to be any way copy files on Windows that would work with
  # spaces and special characters, like + signs. Tried copy, xcopy, robocopy
  QMAKE_INSTALL_FILE='\"C:\Program Files\Git\usr\bin\install.exe\"'
}
