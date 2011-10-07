include(../multitude.pri)
include(../library.pri)

TEMPLATE=subdirs
QMAKE_EXTRA_TARGETS += first

V8 += library=shared

contains(QMAKE_HOST.arch, x86_64) {
  V8 += arch=x64
} else {
  V8 += arch=ia32
}
CONFIG(release, debug|release) {
  V8LIB=$${LIB_PREFIX}v8
  TARGET=$${V8LIB}.$$SHARED_LIB_SUFFIX
  V8 += mode=release
} else {
  V8LIB=$${LIB_PREFIX}v8_g
  V8LIB_OUT=$${LIB_PREFIX}v8_d
  TARGET=$${V8LIB}.$$SHARED_LIB_SUFFIX
  V8 += verbose=on mode=debug
}

win32 {
  DEST=$$replace(DESTDIR, /, \\)\\$$V8LIB_OUT
  first.commands = if not exist $$TARGET scons env='"PATH:%PATH%,INCLUDE:%INCLUDE%,LIB:%LIB%"' $$V8 $$TARGET -j4 && copy $${V8LIB}.dll $${DEST}.dll && copy $${V8LIB}.lib $${DEST}.lib
}
!win32 {
  DEST=$$DESTDIR/$${V8LIB}.$$SHARED_LIB_SUFFIX
  first.commands = if test ! -s $$TARGET; then scons $$V8 $$TARGET -j4; fi && cp $$TARGET $$DEST
}

clean.commands = scons -c $$TARGET
MAKE_EXTRA_TARGETS += clean
