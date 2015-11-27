lessThan(QT_MAJOR_VERSION, 5) {
  error("Cornerstone SDK requires Qt 5")
}

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

# Always include Deft
CONFIG += enable-deft
# On taction tracker build, enable cuda backend but disable js by default
# JS can still be enabled with enable-js
enable-taction:CONFIG += enable-deft-cuda
enable-taction:CONFIG += disable-js
enable-taction:DEFINES += MULTITACTION_FIRMWARE

# deft can be disabled with -config disable-deft
disable-deft:CONFIG -= enable-deft enable-deft-cuda
disable-deft-cuda:CONFIG -= enable-deft-cuda
disable-deft-opencl:CONFIG -= enable-deft-opencl

# JS is enabled by default
!disable-js:CONFIG += enable-js
enable-js:CONFIG -= disable-js
enable-js:DEFINES += CORNERSTONE_JS=1

# CEF is enabled by default
!disable-cef:CONFIG += enable-cef

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
enable-js:LIB_V8 = -lv8-multitouch1 -lnode-multitouch1

#
# Platform specific: GNU Linux
#
linux-*{

  # Must define this manually on Linux & Windows
  QMAKE_EXTENSION_SHLIB=so

  QMAKE_LIBDIR += /usr/lib/nvidia-current

  QMAKE_LIBDIR += $$PWD/Linux/lib

  exists(/opt/multitouch-libav2/include/libavcodec/avcodec.h) {
    MULTI_FFMPEG_LIBS = -L/opt/multitouch-libav2/lib -lavcodec-multitouch2 -lavutil-multitouch2 -lavformat-multitouch2 -lavdevice-multitouch2 -lavfilter-multitouch2 -lswscale-multitouch2
    INCLUDEPATH += /opt/multitouch-libav2/include
  }

  enable-js {
    QMAKE_LIBDIR += /opt/multitouch-nodejs-1/lib
    INCLUDEPATH += /opt/multitouch-nodejs-1/include
  }

  contains(DOCUMENTER,yes) {
    message(Enabling document generator)
    DEFINES += MULTI_DOCUMENTER=1
  }

  defineTest(gccVersionCheck) {
    COMPILER_OUTPUT=$$system($$1 -dumpversion 2>/dev/null)
    COMPILER_V1 = $$section(COMPILER_OUTPUT, ".", 0, 0)
    COMPILER_V2 = $$section(COMPILER_OUTPUT, ".", 1, 1)
    greaterThan(COMPILER_V1, $$2): return(true)
    equals(COMPILER_V1, $$2) {
      greaterThan(COMPILER_V2, $$3): return(true)
      equals(COMPILER_V2, $$3): return(true)
    }
    return(false)
  }
  defineTest(checkCompiler) {
    gccVersionCheck($$1, 4, 6): return(true)
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
  !checkCompiler($$QMAKE_CXX) {
    error("$$QMAKE_CXX is too old, need at least g++ 4.6")
  }
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
  CORNERSTONE_DEPS_DIR=/opt/multitouch-$$CORNERSTONE_VERSION_STR

  QMAKE_LFLAGS += -Wl,-rpath,/opt/cornerstone-$$CORNERSTONE_VERSION_STR/lib
  QMAKE_MACOSX_DEPLOYMENT_TARGET=10.7
  QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++ -Wno-self-assign -Wno-overloaded-virtual -Qunused-arguments
  QMAKE_OBJECTIVE_CFLAGS += -std=c++11 -stdlib=libc++
  QMAKE_LFLAGS += -stdlib=libc++

  QMAKE_CFLAGS_WARN_ON =
  QMAKE_CXXFLAGS_WARN_ON =

  QMAKE_LIBDIR += $$PWD/OSX/lib

  LIB_OPENCL = -framework,OpenCL
  LIB_OPENGL = -framework,OpenGL

  system([ `uname -r | cut -d . -f1` -eq 10 ] ):DEFINES+=RADIANT_OSX_SNOW_LEOPARD
  system([ `uname -r | cut -d . -f1` -eq 11 ] ):DEFINES+=RADIANT_OSX_LION
  system([ `uname -r | cut -d . -f1` -eq 12 ] ):DEFINES+=RADIANT_OSX_MOUNTAIN_LION
  system([ `uname -r | cut -d . -f1` -eq 14 ] ):DEFINES+=RADIANT_OSX_YOSEMITE

  # By default pkg-config support is disabled on OSX, re-enable it here
  QT_CONFIG -= no-pkg-config
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

    # For official builds by MultiTouch Ltd.
    CORNERSTONE_DEPS_PATH=C:\\Cornerstone-$${CORNERSTONE_VERSION_STR}-deps

    exists($$CORNERSTONE_DEPS_PATH) {
      INCLUDEPATH += $$CORNERSTONE_DEPS_PATH/libav/include
      LIBS += -L$$CORNERSTONE_DEPS_PATH/libav/bin
      enable-js {
        INCLUDEPATH += $$CORNERSTONE_DEPS_PATH/node/include
        LIBS += -L$$CORNERSTONE_DEPS_PATH/node/bin
        QMAKE_LIBDIR += $$CORNERSTONE_DEPS_PATH/node/lib
      }
    } else {
      # Builds from distributed source code
      CORNERSTONE_DEPS_PATH=C:\\Cornerstone-SDK-$${CORNERSTONE_VERSION_STR}

      exists($$CORNERSTONE_DEPS_PATH) {
        INCLUDEPATH += $$CORNERSTONE_DEPS_PATH/include
        LIBS += -L$$CORNERSTONE_DEPS_PATH/bin
        QMAKE_LIBDIR += $$CORNERSTONE_DEPS_PATH/lib
      } else {
        error(No dependency package found. Build requires the Cornerstone SDK package ($$CORNERSTONE_DEPS_PATH) to proceed.)
      }
    }

    # These libs have an extra extension for debug builds
    CONFIG(debug,debug|release) {
      LIB_OPENGL = -lglew$${CORNERSTONE_LIB_SUFFIX} -lglu32 -lopengl32
      enable-js:LIB_V8 = -lv8-multitouch1_d -lnode-multitouch1_d
    }

  # output pdbs for release builds as well. Otherwise, profiling is impossible
  CONFIG(release):pdb-in-release {
    QMAKE_LFLAGS += /MAP
    QMAKE_CFLAGS += /Zi
    QMAKE_LFLAGS += /debug /opt:ref
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

QMAKE_LIBDIR += $$shadowed($$PWD)/lib

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

enable-cef:include(../WebBrowserCef/cef.pri)

disable-deprecation-warnings {
  *clang* | *g++* {
    QMAKE_CXXFLAGS += -Wno-deprecated-declarations
  }
  *msvc* {
    QMAKE_CXXFLAGS += -wd4996
  }
}

*g++*:QMAKE_LFLAGS += -Wl,--exclude-libs,ALL

!enable-drm {
  *clang* | *g++* {
    QMAKE_CXXFLAGS_RELEASE += -g
  }
}
