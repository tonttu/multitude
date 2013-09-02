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

# We need C++11 to compile
!macx {
  *g++*:QMAKE_CXXFLAGS += -std=c++0x
  *clang*:QMAKE_CXXFLAGS += -std=c++11 -Qunused-arguments
}

# JS is enabled by default
!disable-js:CONFIG += enable-js
enable-js:DEFINES += CORNERSTONE_JS=1

widget-profiler:DEFINES += MULTI_WIDGET_PROFILER=1

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

withbundles = $$(MULTI_BUNDLES)

MULTI_FFMPEG_LIBS = -lavdevice -lavcodec -lavutil -lavformat -lavfilter -lswscale

CORNERSTONE_LIB_SUFFIX = .$${CORNERSTONE_VERSION_STR}

# On Windows, add _d for debug builds
win32 {
  CONFIG(debug,debug|release) {
    CORNERSTONE_LIB_SUFFIX = .$${CORNERSTONE_VERSION_STR}_d
  }
}

# exists(/usr/local/lib/libftd2xx.so)|exists(/opt/multitouch-$$CORNERSTONE_VERSION_STR/lib/libftd2xx.dylib) {
# message(FTD2XX support detected.)
# !win32:CONFIG += with-ftd2xx
with-ftd2xx {
  LIB_FTD2XX = -lftd2xx
  WITH_FTD2XX = yes
  DEFINES += MULTI_WITH_FTD2XX=1
}

LIB_OPENCL = -lOpenCL
LIB_OPENGL = -lglew$${CORNERSTONE_LIB_SUFFIX} -lGLU -lGL
INCLUDEPATH += $$PWD/ThirdParty/glew/include

LIB_POETIC = -lPoetic$${CORNERSTONE_LIB_SUFFIX}
LIB_LUMINOUS = -lLuminous$${CORNERSTONE_LIB_SUFFIX}
LIB_NIMBLE = -lNimble$${CORNERSTONE_LIB_SUFFIX}
LIB_RADIANT = -lRadiant$${CORNERSTONE_LIB_SUFFIX} $$LIB_FTD2XX
LIB_VIDEODISPLAY = -lVideoDisplay$${CORNERSTONE_LIB_SUFFIX}
LIB_VALUABLE = -lValuable$${CORNERSTONE_LIB_SUFFIX}
LIB_PATTERNS = -lPatterns$${CORNERSTONE_LIB_SUFFIX}
LIB_SQUISH = -lSquish$${CORNERSTONE_LIB_SUFFIX}
LIB_RESONANT = -lResonant$${CORNERSTONE_LIB_SUFFIX}
enable-js:LIB_V8 = -lv8 -lnode

#
# Platform specific: GNU Linux
#
linux-*{

  # Must define this manually on Linux & Windows
  QMAKE_EXTENSION_SHLIB=so

  QMAKE_LIBDIR += /usr/lib/nvidia-current

  QMAKE_LIBDIR += $$PWD/Linux/lib

  exists(/opt/multitouch-libav1/include/libavcodec/avcodec.h) {
    MULTI_FFMPEG_LIBS = -L/opt/multitouch-libav1/lib -lavcodec-multitouch1 -lavutil-multitouch1 -lavformat-multitouch1 -lavdevice-multitouch1 -lavfilter-multitouch1 -lswscale-multitouch1
    INCLUDEPATH += /opt/multitouch-libav1/include
  }

  contains(DOCUMENTER,yes) {
    message(Enabling document generator)
    DEFINES += MULTI_DOCUMENTER=1
  }

  defineTest(checkCompiler) {
    COMPILER_OUTPUT=$$system($$1 -dumpversion 2>/dev/null)
    COMPILER_V1 = $$section(COMPILER_OUTPUT, ".", 0, 0)
    COMPILER_V2 = $$section(COMPILER_OUTPUT, ".", 1, 1)
    greaterThan(COMPILER_V1, 4): return(true)
    greaterThan(COMPILER_V1, 3): greaterThan(COMPILER_V2, 5): return(true)
    return(false)
  }
  !checkCompiler($$QMAKE_CXX) {
    checkCompiler(g++-4.6): QMAKE_CXX=g++-4.6
    else:checkCompiler(g++-4.7): QMAKE_CXX=g++-4.7
    else:checkCompiler(g++-4.8): QMAKE_CXX=g++-4.8
  }
  !checkCompiler($$QMAKE_CC) {
    checkCompiler(gcc-4.6): QMAKE_CC=gcc-4.6
    else:checkCompiler(gcc-4.7): QMAKE_CC=gcc-4.7
    else:checkCompiler(gcc-4.8): QMAKE_CC=gcc-4.8
  }
  !checkCompiler($$QMAKE_LINK): QMAKE_LINK=$$QMAKE_CXX
}

contains(MEMCHECK,yes) {
  message(Using Radiant::MemCheck)
  DEFINES += MULTI_MEMCHECK=1
  linux:LIBS += -rdynamic
}

#
# Platform specific: Apple OS X
#
macx {
  CORNERSTONE_DEPS_DIR = /opt/multitouch
  exists(/opt/multitouch-$$CORNERSTONE_VERSION_MAJOR/include):CORNERSTONE_DEPS_DIR=/opt/multitouch-$$CORNERSTONE_VERSION_MAJOR
  exists(/opt/multitouch-$${CORNERSTONE_VERSION_MAJOR}.$${CORNERSTONE_VERSION_MINOR}/include):CORNERSTONE_DEPS_DIR=/opt/multitouch-$${CORNERSTONE_VERSION_MAJOR}.$${CORNERSTONE_VERSION_MINOR}
  exists(/opt/multitouch-$${CORNERSTONE_VERSION}/include):CORNERSTONE_DEPS_DIR=/opt/multitouch-$$CORNERSTONE_VERSION
  exists(/opt/multitouch-$${CORNERSTONE_VERSION_STR}/include):CORNERSTONE_DEPS_DIR=/opt/multitouch-$$CORNERSTONE_VERSION_STR

  QMAKE_LFLAGS += -Wl,-rpath,/opt/cornerstone-$$CORNERSTONE_VERSION_STR/lib
  QMAKE_MACOSX_DEPLOYMENT_TARGET=10.7
  QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++ -Wno-self-assign -Wno-overloaded-virtual -Qunused-arguments
  QMAKE_CC = clang -std=c++11 -stdlib=libc++
  QMAKE_LFLAGS += -stdlib=libc++

  QMAKE_CFLAGS_WARN_ON =
  QMAKE_CXXFLAGS_WARN_ON =

  QMAKE_LIBDIR += $$PWD/OSX/lib

  LIB_OPENCL = -framework,OpenCL
  LIB_OPENGL = -framework,OpenGL

  system([ `uname -r | cut -d . -f1` -eq 10 ] ):DEFINES+=RADIANT_OSX_SNOW_LEOPARD
  system([ `uname -r | cut -d . -f1` -eq 11 ] ):DEFINES+=RADIANT_OSX_LION
  system([ `uname -r | cut -d . -f1` -eq 12 ] ):DEFINES+=RADIANT_OSX_MOUNTAIN_LION
}

#
# Platform specific: Microsoft Windows
#
win32 {
    # Must define this manually on Linux & Windows
    QMAKE_EXTENSION_SHLIB=dll

    # Try to identify used compiler on Windows (32 vs 64)
    COMPILER_OUTPUT=$$system(cl 2>&1)
    contains(COMPILER_OUTPUT,x64):CONFIG+=win64

    WINPORT_INCLUDE = $$PWD\\Win64x\\include
    INCLUDEPATH += $$PWD\\Win64x\\include
    QMAKE_LIBDIR += $$PWD/Win64x/lib64

    exists("C:\\WinDDK\\7600.16385.1"):DDK_PATH="C:\\WinDDK\\7600.16385.1"
    exists("C:\\Program Files (x86)\\Windows Kits\\8.0\\Include"):DDK_PATH="C:\\Program Files (x86)\\Windows Kits\\8.0\\Include"

    LIB_OPENGL = -lglew$${CORNERSTONE_LIB_SUFFIX} -lglu32 -lopengl32
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

    exists("C:/Cornerstone-deps/libav/include") {
      INCLUDEPATH += "C:/Cornerstone-deps/libav/include"
      LIBS += -L"C:/Cornerstone-deps/libav/bin"
      enable-js {
        INCLUDEPATH += "C:/Cornerstone-deps/node/include"
        LIBS += -L"C:/Cornerstone-deps/node/bin"
        QMAKE_LIBDIR += "C:/Cornerstone-deps/node/lib"
      }
    } else {
      error(Requires the Cornerstone dependency package to compile)
    }

    # These libs have an extra extension for debug builds
    CONFIG(debug,debug|release) {
      LIB_OPENGL = -lglew$${CORNERSTONE_LIB_SUFFIX} -lglu32 -lopengl32
      enable-js:LIB_V8 = -lv8_d -lnode_d
    }
}

#
# Platform specific: Unix (OS X & Linux)
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

disable-deprecation-warnings {
  *clang* | *g++* {
    QMAKE_CXXFLAGS += -Wno-deprecated-declarations
  }
  *msvc* {
    QMAKE_CXXFLAGS += -wd4996
  }
}

*g++*:QMAKE_LFLAGS += -Wl,--exclude-libs,ALL
