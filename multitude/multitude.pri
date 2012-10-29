# Use this to setup global build options & variables.
# This file is to be included by all project files of qmake.
CONFIG += qt
QT += core

CONFIG += link_pkgconfig
CONFIG += thread

CONFIG += embed_manifest_exe

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

macx {
  CORNERSTONE_DEPS_DIR = /opt/multitouch
  exists(/opt/multitouch-$$CORNERSTONE_VERSION_MAJOR/include):CORNERSTONE_DEPS_DIR=/opt/multitouch-$$CORNERSTONE_VERSION_MAJOR
  exists(/opt/multitouch-$${CORNERSTONE_VERSION_MAJOR}.$${CORNERSTONE_VERSION_MINOR}/include):CORNERSTONE_DEPS_DIR=/opt/multitouch-$${CORNERSTONE_VERSION_MAJOR}.$${CORNERSTONE_VERSION_MINOR}
  exists(/opt/multitouch-$$CORNERSTONE_VERSION/include):CORNERSTONE_DEPS_DIR=/opt/multitouch-$$CORNERSTONE_VERSION
  exists(/opt/multitouch-$$CORNERSTONE_VERSION_STR/include):CORNERSTONE_DEPS_DIR=/opt/multitouch-$$CORNERSTONE_VERSION_STR
}

# We need C++11 to compile
!macx:*g++*:QMAKE_CXXFLAGS += -std=c++0x
!macx:*clang*:QMAKE_CXXFLAGS += -std=c++11 -Qunused-arguments
macx {
  QMAKE_LFLAGS += -Wl,-rpath,/opt/cornerstone-$$CORNERSTONE_VERSION_STR/lib
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

# JS is enabled by default
!disable-js {
  CONFIG += enable-js
}

enable-js {
  DEFINES += CORNERSTONE_JS=1
}

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

withbundles = $$(MULTI_BUNDLES)

!mobile*:MULTI_FFMPEG_LIBS = -lavdevice -lavcodec -lavutil -lavformat -lavfilter -lswscale

CORNERSTONE_LIB_SUFFIX = .$${CORNERSTONE_VERSION}

LIB_RESONANT = -lResonant$${CORNERSTONE_LIB_SUFFIX}
LIB_BOX2D = -lBox2D$${CORNERSTONE_LIB_SUFFIX}

!mobile {
  # exists(/usr/local/lib/libftd2xx.so)|exists(/opt/multitouch-$$CORNERSTONE_VERSION_STR/lib/libftd2xx.dylib) {
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
INCLUDEPATH += $$PWD/ThirdParty/glew/include

LIB_POETIC = -lPoetic$${CORNERSTONE_LIB_SUFFIX}
LIB_LUMINOUS = -lLuminous$${CORNERSTONE_LIB_SUFFIX}
LIB_NIMBLE = -lNimble$${CORNERSTONE_LIB_SUFFIX}
LIB_RADIANT = -lRadiant$${CORNERSTONE_LIB_SUFFIX} $$LIB_FTD2XX
!mobile*:LIB_VIDEODISPLAY = -lVideoDisplay$${CORNERSTONE_LIB_SUFFIX}
LIB_VALUABLE = -lValuable$${CORNERSTONE_LIB_SUFFIX}
LIB_PATTERNS = -lPatterns$${CORNERSTONE_LIB_SUFFIX}
LIB_SQUISH = -lSquish$${CORNERSTONE_LIB_SUFFIX}
enable-js:LIB_V8 = -lv8 -lnode

#
# Platform specific: Unix (OSX & linux)
#
unix {
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

  exists($$CORNERSTONE_DEPS_DIR):INCLUDEPATH+=$$CORNERSTONE_DEPS_DIR/include
  exists($$CORNERSTONE_DEPS_DIR):LIBS+=-L$$CORNERSTONE_DEPS_DIR/lib
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
  QMAKE_LIBDIR += $$PWD/OSX/lib

  LIB_OPENCL = -framework,OpenCL
  LIB_OPENGL = -framework,OpenGL

  !mobile* {
    DEFINES += QT_MAC_USE_COCOA Q_OS_MAC64

  # change architecture to x86_64 if snow leopard
  system([ `uname -r | cut -d . -f1` -gt 9 ] )  {
  CONFIG += x86_64

  }

  system([ `uname -r | cut -d . -f1` -eq 10 ] ):DEFINES+=RADIANT_OSX_SNOW_LEOPARD
  system([ `uname -r | cut -d . -f1` -eq 11 ] ):DEFINES+=RADIANT_OSX_LION
  system([ `uname -r | cut -d . -f1` -eq 12 ] ):DEFINES+=RADIANT_OSX_MOUNTAIN_LION
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
      enable-js:INCLUDEPATH += $$PWD\\Win64x\\include\\v8
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
    # Use multiprocessor compilation
    QMAKE_CXXFLAGS += -MP
    
    # These libs have an extra extension for debug builds
    build_pass:CONFIG(debug,debug|release) {
      # TODO There shouldn't be a glew_d library
      LIB_OPENGL = -lglew_d -lglu32 -lopengl32
      LIB_BOX2D = -lBox2D$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_POETIC = -lPoetic$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_STYLISH = -lStylish$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_LUMINOUS = -lLuminous$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_NIMBLE = -lNimble$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_RADIANT = -lRadiant$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_RESONANT = -lResonant$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_VIDEODISPLAY = -lVideoDisplay$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_VALUABLE = -lValuable$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_PATTERNS = -lPatterns$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_SQUISH = -lSquish$${CORNERSTONE_LIB_SUFFIX}_d
      enable-js:LIB_V8 = -lv8_d -lnode_d
	}
}

MULTI_VIDEO_LIBS = $$LIB_RESONANT $$LIB_VIDEODISPLAY

QMAKE_LIBDIR += $$PWD/lib

# Disable asserts in release mode
CONFIG(release, debug|release) {
  DEFINES += NDEBUG
}

DEFINES += USING_V8_SHARED
VERSION=

# Enable memchecking
contains(MEMCHECK,yes) {
  message(Using Radiant::MemCheck)
  DEFINES += MULTI_MEMCHECK=1
  linux:LIBS += -rdynamic
}
