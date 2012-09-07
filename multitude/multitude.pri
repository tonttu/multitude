# Use this to setup global build options & variables.
# This file is to be included by all project files of qmake.
CONFIG += qt
QT += core

CONFIG += link_pkgconfig
CONFIG += thread

CONFIG += embed_manifest_exe

# We need C++11 to compile
!macx:*g++*:QMAKE_CXXFLAGS += -std=c++0x
!macx:*clang*:QMAKE_CXXFLAGS += -std=c++11 -Qunused-arguments
macx {
	QMAKE_MACOSX_DEPLOYMENT_TARGET=10.7
  QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++ -Wno-self-assign -Wno-overloaded-virtual -Qunused-arguments
        QMAKE_CC = clang -std=c++11 -stdlib=libc++
        QMAKE_LFLAGS += -stdlib=libc++

  QMAKE_CFLAGS_WARN_ON =
  QMAKE_CXXFLAGS_WARN_ON =
}

iphone {
  CONFIG += mobile
  CONFIG += ios
}

ios {
  CONFIG += mobile
  QMAKE_CXXFLAGS -= -std=gnu99
  QMAKE_CXXFLAGS -= -fobjc-legacy-dispatch
}

mobile {
  message(Mobile device compilation)
  # For QString::toStdWString
  DEFINES += QT_STL=1
  CONFIG += render_es2
}

CONFIG += render_es2

render_es2 {
  DEFINES += CORNERSTONE_RENDER_ES2=1
}

enable-js {
  DEFINES += CORNERSTONE_JS=1
}

INCLUDEPATH += $$PWD
!mobile:INCLUDEPATH += $$PWD/v8/include
DEPENDPATH += $$PWD

withbundles = $$(MULTI_BUNDLES)

!mobile*:MULTI_FFMPEG_LIBS = -lavdevice -lavcodec -lavutil -lavformat -lavfilter -lswscale

# 1.9.2-rc2
CORNERSTONE_VERSION_STR = $$cat(../VERSION)
# 1.9.2
CORNERSTONE_VERSION = $$section(CORNERSTONE_VERSION_STR, "-", 0, 0)
# 1
CORNERSTONE_VERSION_MAJOR = $$section(CORNERSTONE_VERSION, ".", 0, 0)
# 9
CORNERSTONE_VERSION_MINOR = $$section(CORNERSTONE_VERSION, ".", 1, 1)
# 2
CORNERSTONE_VERSION_PATCH = $$section(CORNERSTONE_VERSION, ".", 2, 2)

win32 {
  CORNERSTONE_LIB_SUFFIX = .$${CORNERSTONE_VERSION}
}
!win32 {
  CORNERSTONE_LIB_SUFFIX =
}

LIB_RESONANT = -lResonant$${CORNERSTONE_LIB_SUFFIX}
LIB_BOX2D = -lBox2D$${CORNERSTONE_LIB_SUFFIX}

!mobile {
  # exists(/usr/local/lib/libftd2xx.so)|exists(/opt/multitouch/lib/libftd2xx.dylib) {
  # message(FTD2XX support detected.)
  # !win32:CONFIG += with-ftd2xx
  with-ftd2xx {
    LIB_FTD2XX = -lftd2xx
    WITH_FTD2XX = yes
    DEFINES += MULTI_WITH_FTD2XX=1
  }
}

LIB_OPENCL = -lOpenCL
LIB_OPENGL = -lglew -lGLU -lGL
INCLUDEPATH += $$PWD/3rdparty/glew-1.9.0/include

LIB_POETIC = -lPoetic$${CORNERSTONE_LIB_SUFFIX}
LIB_FLUFFY = -lFluffy$${CORNERSTONE_LIB_SUFFIX}
LIB_LUMINOUS = -lLuminous$${CORNERSTONE_LIB_SUFFIX}
LIB_NIMBLE = -lNimble$${CORNERSTONE_LIB_SUFFIX}
LIB_RADIANT = -lRadiant$${CORNERSTONE_LIB_SUFFIX} $$LIB_FTD2XX
!mobile*:LIB_VIDEODISPLAY = -lVideoDisplay$${CORNERSTONE_LIB_SUFFIX}
LIB_VALUABLE = -lValuable$${CORNERSTONE_LIB_SUFFIX}
LIB_PATTERNS = -lPatterns$${CORNERSTONE_LIB_SUFFIX}
LIB_SQUISH = -lSquish$${CORNERSTONE_LIB_SUFFIX}
enable-js:LIB_V8 = -lv8

#
# Platform specific: Unix (OSX & linux)
#
unix {
  VERSION = $${CORNERSTONE_VERSION}

  # Use ccache if available


  exists(/opt/local/bin/ccache) {
    # For Macports + QtCreator users:
    QMAKE_CXX=/opt/local/bin/ccache $$QMAKE_CXX
    QMAKE_CC=/opt/local/bin/ccache $$QMAKE_CC
  }
  else {
    system(which ccache > /dev/null 2>&1) {
      QMAKE_CXX=ccache $$QMAKE_CXX
      QMAKE_CC=ccache $$QMAKE_CC
    }
  }

  exists(/opt/multitouch):INCLUDEPATH+=/opt/multitouch/include
  exists(/opt/multitouch):LIBS+=-L/opt/multitouch/lib
}

#
# Platform specific: GNU Linux
#
linux-*{
  LIB_PREFIX = lib
  SHARED_LIB_SUFFIX = so

  contains(USEGLEW,no) {
    DEFINES += MULTI_WITHOUT_GLEW=1
  }

  QMAKE_LIBDIR += /usr/lib/nvidia-current

  !mobile:QMAKE_LIBDIR += $$PWD/Linux/lib

  exists(/opt/multitouch-ffmpeg/include/libavcodec/avcodec.h) {
    MULTI_FFMPEG_LIBS = -L/opt/multitouch-ffmpeg/lib -lavcodec-multitouch -lavutil-multitouch -lavformat-multitouch -lavdevice-multitouch -lavfilter-multitouch -lswscale-multitouch
    INCLUDEPATH += /opt/multitouch-ffmpeg/include
  }

  build_pass:CONFIG(debug,debug|release) {
    # Debug builds run with Electric Fence for extra memory testing
    # Check the efence manpages for using different environment variables
    LIBS += -lefence
  }
  contains(DOCUMENTER,yes) {
    message(Enabling document generator)
    DEFINES += MULTI_DOCUMENTER=1
  }
}

contains(MEMCHECK,yes) {
  message(Using Radiant::MemCheck)
  DEFINES += MULTI_MEMCHECK=1
  linux:LIBS += -rdynamic
}

macx*|mobile* {
  LIB_PREFIX = lib
  !mobile*:SHARED_LIB_SUFFIX = dylib
  # Fake SHARED_LIB_SUFFIX, since iOS does not accept shared libs
  mobile*:SHARED_LIB_SUFFIX = a
  # For Deft (which depends on MultiTouch)
  # LIBS += -undefined dynamic_lookup

  # Frameworks on OS X don't respect QMAKE_LIBDIR
  !mobile:QMAKE_LFLAGS += -F$$PWD/lib -L$$PWD/OSX/lib

  # withbundles = $$(MULTI_BUNDLES)
  withbundles = YES

  LIB_OPENCL = -framework,OpenCL
  LIB_OPENGL = -framework,OpenGL

  !mobile* {
    LIB_POETIC = -framework,Poetic
    LIB_FLUFFY = -framework,Fluffy
    LIB_LUMINOUS = -framework,Luminous
    LIB_NIMBLE = -framework,Nimble
    LIB_RADIANT = -framework,Radiant
    LIB_RESONANT = -framework,Resonant -lsndfile
    LIB_VALUABLE = -framework,Valuable
    LIB_VIDEODISPLAY = -framework,VideoDisplay
    LIB_PATTERNS = -framework,Patterns

    DEFINES += QT_MAC_USE_COCOA Q_OS_MAC64

    DEFINES += QT_MAC_USE_COCOA Q_OS_MAC64

    # DEFINES += __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=1050

    contains(withbundles,YES) {

  # change architecture to x86_64 if snow leopard
  system([ `uname -r | cut -d . -f1` -gt 9 ] )  {
  CONFIG += x86_64

  }

  system([ `uname -r | cut -d . -f1` -eq 10 ] ):DEFINES+=RADIANT_OSX_SNOW_LEOPARD
  system([ `uname -r | cut -d . -f1` -eq 11 ] ):DEFINES+=RADIANT_OSX_LION
  system([ `uname -r | cut -d . -f1` -eq 12 ] ):DEFINES+=RADIANT_OSX_MOUNTAIN_LION
}
}
}

#
# Platform specific: Microsoft Windows
#
win32 {
    # Try to identify used compiler on Windows (32 vs 64)
    COMPILER_OUTPUT=$$system(cl 2>&1)
    contains(COMPILER_OUTPUT,x64):CONFIG+=win64

    win64 {
      WINPORT_INCLUDE = $$PWD\\Win64x\\include
      INCLUDEPATH += $$PWD\\Win64x\\include
      INCLUDEPATH += $$PWD/../multitude/Win64x/include/ffmpeg
      QMAKE_LIBDIR += $$PWD/Win64x/lib64
    } else {
      WINPORT_INCLUDE = $$PWD\\Win32x\\include
      INCLUDEPATH += $$PWD\\Win32x\\include
      INCLUDEPATH += $$PWD/../multitude/Win32x/include/ffmpeg
      QMAKE_LIBDIR += $$PWD\\Win32x\\lib32
    }

    LIB_PREFIX =
    SHARED_LIB_SUFFIX = dll

    DDK_PATH="C:\\WinDDK\\7600.16385.1"

    LIB_OPENGL = -lglew -lglu32 -lopengl32
    # Make VS a bit less spammy
    QMAKE_CXXFLAGS += -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS
    # conversion from 'size_t' to 'type', possible loss of data
    QMAKE_CXXFLAGS += -wd4267
    # conversion from 'type1' to 'type2', possible lost of data
    QMAKE_CXXFLAGS += -wd4244
    # class 'type' needs to have dll-interface to be used by clients of class 'type2'
    QMAKE_CXXFLAGS += -wd4251
    # this used in base member initializer list
    QMAKE_CXXFLAGS += -wd4355
    # Truncation from double to float
    QMAKE_CXXFLAGS += -wd4305
    # Signed/unsigned mismatch
    QMAKE_CXXFLAGS += -wd4018
    # Use the non-standard math defines from math.h
    QMAKE_CXXFLAGS += -D_USE_MATH_DEFINES
    
    # These libs have an extra extension for debug builds
    build_pass:CONFIG(debug,debug|release) {
      # TODO There shouldn't be a glew_d library
      LIB_OPENGL = -lglew_d -lglu32 -lopengl32
      LIB_BOX2D = -lBox2D$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_POETIC = -lPoetic$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_FLUFFY = -lFluffy$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_LUMINOUS = -lLuminous$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_NIMBLE = -lNimble$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_RADIANT = -lRadiant$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_RESONANT = -lResonant$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_VIDEODISPLAY = -lVideoDisplay$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_VALUABLE = -lValuable$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_PATTERNS = -lPatterns$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_SQUISH = -lSquish$${CORNERSTONE_LIB_SUFFIX}_d
      enable-js:LIB_V8 = -lv8_d
	}
}

MULTI_VIDEO_LIBS = $$LIB_RESONANT $$LIB_VIDEODISPLAY

QMAKE_LIBDIR += $$PWD/lib

# message(QT version is $${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION}.$${QT_PATCH_VERSION})
# mobile*:DEFINES += __IPHONE_OS_VERSION_MIN_REQUIRED=40100

contains(QT_MAJOR_VERSION,4) {

  contains(QT_MINOR_VERSION,5) || contains(QT_MINOR_VERSION,6) || contains(QT_MINOR_VERSION,7) {
    HAS_QT_45=YES
    DEFINES += USE_QT45
  }
}
# Disable asserts in release mode
CONFIG(release, debug|release) {
  DEFINES += NDEBUG
}

DEFINES += USING_V8_SHARED

# Tommi's hack
exists(/opt/local/libexec/llvm-3.2/bin/clang_not) {
  # This section overrides g++, and selects clang instead. Warning flags are modified to
  # reduce the error spam. Without these we get a constant stream of warnings for
  # each Q_OBJECT macro (which is many).
  REDUCE_CLANG_WARNINGS = -Wno-self-assign -Wno-overloaded-virtual -Qunused-arguments
  QMAKE_CC  = /opt/local//libexec/llvm-3.2/bin/clang -std=c++11 $$REDUCE_CLANG_WARNINGS
  QMAKE_CXX = /opt/local//libexec/llvm-3.2/bin/clang++ -std=c++11 $$REDUCE_CLANG_WARNINGS
  QMAKE_LINK       = $$QMAKE_CXX /opt/local//lib/gcc47/libstdc++.6.dylib
  QMAKE_CFLAGS_WARN_ON =
  QMAKE_CXXFLAGS_WARN_ON =
  CLANG_INCLUDEPATH += -I/opt/local//include/gcc47/c++/ -I/opt/local//include/gcc47/c++/x86_64-apple-darwin11/
  QMAKE_CXX += $$CLANG_INCLUDEPATH
  QMAKE_CC  += $$CLANG_INCLUDEPATH
}


# Enable memchecking
contains(MEMCHECK,yes) {
  message(Using Radiant::MemCheck)
  DEFINES += MULTI_MEMCHECK=1
  linux:LIBS += -rdynamic
}
