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
debug {
  TARGET=libv8_g.so
  V8 += verbose=on mode=debug
} else {
  TARGET=libv8.so
  V8 += mode=release
}

first.commands = if test ! -f $$TARGET; then scons $$V8 library -j4; fi && cp $$TARGET $$DESTDIR/libv8.so

MAKE_EXTRA_TARGETS += clean
clean.commands = scons -c
