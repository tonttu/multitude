include(../multitude.pri)
include(../library.pri)

TEMPLATE=subdirs
QMAKE_EXTRA_TARGETS += first

V8 += library=shared snapshot=on

contains(QMAKE_HOST.arch, x86_64) {
  V8 += arch=x64
} else {
  V8 += arch=ia32
}
CONFIG(release, debug|release) {
  TARGET=libv8.$$SHARED_LIB_SUFFIX
  V8 += mode=release
} else {
  TARGET=libv8_g.$$SHARED_LIB_SUFFIX
  V8 += verbose=on mode=debug
}

first.commands = if test ! -s $$TARGET; then scons $$V8 library -j4; fi && cp $$TARGET $$DESTDIR/libv8.$$SHARED_LIB_SUFFIX

MAKE_EXTRA_TARGETS += clean
clean.commands = scons -c
