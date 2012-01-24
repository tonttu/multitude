include(../multitude.pri)
include(../library.pri)

TEMPLATE=subdirs
QMAKE_EXTRA_TARGETS += first

!mobile*:V8 += library=shared

macx {
  # This is not 100% correct, but for now we can assume that all OSX is 64-bits
  mobile* {
    V8 += arch=ia32  
  } else {
    V8 += arch=x64
  }
}
else {
  contains(QMAKE_HOST.arch, x86_64) {
    V8 += arch=x64
  } else {
    V8 += arch=ia32
  }
}

CONFIG(release, debug|release) {
  V8LIB=$${LIB_PREFIX}v8
  V8LIB_OUT=$${V8LIB}
  !mobile*:TARGET=$${V8LIB}.$$SHARED_LIB_SUFFIX
  mobile*:TARGET=$${V8LIB}.a
  V8 += mode=release
} else {
  win32 {
    V8LIB_OUT=$${LIB_PREFIX}v8_d
  }
  !win32 {
    V8LIB_OUT=$${LIB_PREFIX}v8
  }
  V8LIB=$${LIB_PREFIX}v8_d
  !mobile*:TARGET=$${V8LIB}.$$SHARED_LIB_SUFFIX
  mobile*:TARGET=$${V8LIB}.a
  V8 += verbose=on mode=debug
}

stuff.path = /lib
INSTALLS += stuff
macx:stuff.path = /Applications/MultiTouch

linux-* {
  DEST=$$DESTDIR/$${V8LIB_OUT}.$$SHARED_LIB_SUFFIX
  # Running application will crash with sigbus without --remove-destination
  first.commands = if test ! -s $$TARGET; then scons $$V8 $$TARGET -j4; fi && cp --remove-destination $$TARGET $$DEST
  stuff.files += $$DEST
}
macx*|mobile* {
  DEST=$$DESTDIR/$${V8LIB_OUT}.$$SHARED_LIB_SUFFIX
  # Running application will crash with sigbus without --remove-destination
  first.commands = if test ! -s $$TARGET; then scons $$V8 $$TARGET -j4; fi && cp $$TARGET $$DEST
}

macx {
  DEST=$$DESTDIR/$${V8LIB_OUT}.$$SHARED_LIB_SUFFIX
  # Running application will crash with sigbus without --remove-destination, but it does not exist on OSX
  first.commands = if test ! -s $$TARGET; then scons $$V8 $$TARGET -j4; fi && cp $$TARGET $$DEST
  stuff.files += $$DEST
}

clean.commands = scons -c $$TARGET
MAKE_EXTRA_TARGETS += clean
