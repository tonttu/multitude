# Use this to setup global build options & variables.
# This file is to be included by all project files of qmake.
CONFIG += qt
QT += core

CONFIG += link_pkgconfig
CONFIG += thread

CONFIG += embed_manifest_exe

c++11 {
  !win32 {
    message(Enabling C++11)
    QMAKE_CXXFLAGS += -std=c++0x
  }
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
  CONFIG += without-js
}

INCLUDEPATH += $$PWD
!mobile:INCLUDEPATH += $$PWD/v8/include
DEPENDPATH += $$PWD

withbundles = $$(MULTI_BUNDLES)

!mobile*:MULTI_FFMPEG_LIBS = -lavcodec -lavutil -lavformat

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
LIB_OPENCL = -lOpenCL
LIB_OPENGL = -lGL -lGLU

LIB_POETIC = -lPoetic$${CORNERSTONE_LIB_SUFFIX}
LIB_FLUFFY = -lFluffy$${CORNERSTONE_LIB_SUFFIX}
LIB_LUMINOUS = -lLuminous$${CORNERSTONE_LIB_SUFFIX}
LIB_NIMBLE = -lNimble$${CORNERSTONE_LIB_SUFFIX}
LIB_RADIANT = -lRadiant$${CORNERSTONE_LIB_SUFFIX}
!mobile*:LIB_SCREENPLAY = -lScreenplay$${CORNERSTONE_LIB_SUFFIX}
!mobile*:LIB_VIDEODISPLAY = -lVideoDisplay$${CORNERSTONE_LIB_SUFFIX}
LIB_VALUABLE = -lValuable$${CORNERSTONE_LIB_SUFFIX}
LIB_PATTERNS = -lPatterns$${CORNERSTONE_LIB_SUFFIX}
LIB_SQUISH = -lSquish$${CORNERSTONE_LIB_SUFFIX}
!without-js:LIB_V8 = -lv8

#
# Platform specific: Unix (OSX & linux)
#
unix {
  VERSION = $${CORNERSTONE_VERSION}
  
  # Use ccache if available
  exists(/usr/bin/ccache):QMAKE_CXX=ccache g++
  exists(/sw/bin/ccache):QMAKE_CXX=/sw/bin/ccache g++
  exists(/opt/local/bin/ccache):QMAKE_CXX=/opt/local/bin/ccache g++

  exists(/opt/multitouch):INCLUDEPATH+=/opt/multitouch/include
  exists(/opt/multitouch):LIBS+=-L/opt/multitouch/lib
}

#
# Platform specific: GNU Linux
#
linux-*{
  vivid {
    QMAKE_LIBDIR += $$(FBX_SDK)/lib/gcc4
    LIB_VIVID = -lVivid -lfbxsdk_20113_1_x64
  }

  LIB_PREFIX = lib
  SHARED_LIB_SUFFIX = so

  contains(USEGLEW,no) {
    DEFINES += MULTI_WITHOUT_GLEW=1
  } else {
    LIB_GLEW=-lGLEW
  }

  QMAKE_LIBDIR += /usr/lib/nvidia-current

  exists(/opt/multitouch-ffmpeg/include/libavcodec/avcodec.h) {
    MULTI_FFMPEG_LIBS = -L/opt/multitouch-ffmpeg/lib -lavcodec-multitouch -lavutil-multitouch -lavformat-multitouch
    INCLUDEPATH += /opt/multitouch-ffmpeg/include
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
  QMAKE_LFLAGS += -F$$PWD/lib

  # withbundles = $$(MULTI_BUNDLES)
  withbundles = YES

  LIB_OPENCL = -framework,OpenCL
  LIB_OPENGL = -framework,OpenGL
  # LIB_GLEW = -lGLEW

  !mobile* {
    LIB_POETIC = -framework,Poetic
    LIB_FLUFFY = -framework,Fluffy
    LIB_LUMINOUS = -framework,Luminous
    LIB_NIMBLE = -framework,Nimble
    LIB_RADIANT = -framework,Radiant
    LIB_RESONANT = -framework,Resonant -lsndfile
    LIB_SCREENPLAY = -framework,Screenplay
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
      QMAKE_LIBDIR += $$PWD\\Win64x\\lib64
      LIB_GLEW = -lglew64
    } else {
      WINPORT_INCLUDE = $$PWD\\Win32x\\include
      INCLUDEPATH += $$PWD\\Win32x\\include
      INCLUDEPATH += $$PWD/../multitude/Win32x/include/ffmpeg
      QMAKE_LIBDIR += $$PWD\\Win32x\\lib32
      LIB_GLEW = -lglew32
    }

    LIB_PREFIX =
    SHARED_LIB_SUFFIX = dll

    DDK_PATH="C:\\WinDDK\\7600.16385.1"

    LIB_OPENGL = -lopengl32 -lglu32
    QMAKE_CXXFLAGS += -D_CRT_SECURE_NO_WARNINGS -wd4244 -wd4251 -wd4355

    # These libs have an extra extension for debug builds
    build_pass:CONFIG(debug,debug|release) {
      LIB_BOX2D = -lBox2D$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_POETIC = -lPoetic$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_FLUFFY = -lFluffy$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_LUMINOUS = -lLuminous$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_NIMBLE = -lNimble$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_RADIANT = -lRadiant$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_RESONANT = -lResonant$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_SCREENPLAY = -lScreenplay$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_VIDEODISPLAY = -lVideoDisplay$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_VALUABLE = -lValuable$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_PATTERNS = -lPatterns$${CORNERSTONE_LIB_SUFFIX}_d
      LIB_SQUISH = -lSquish$${CORNERSTONE_LIB_SUFFIX}_d
      !mobile:LIB_V8 = -lv8_d
	}
}

MULTI_VIDEO_LIBS = $$LIB_SCREENPLAY $$LIB_RESONANT $$LIB_VIDEODISPLAY

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
build_pass:CONFIG(release, debug|release) {
  DEFINES += NDEBUG
}

DEFINES += USING_V8_SHARED

# Compiler detection
c++11 {
  !win32 {
    message(Enabling C++11)
    QMAKE_CXXFLAGS += -std=c++0x
  }
}

# Enable memchecking
contains(MEMCHECK,yes) {
  message(Using Radiant::MemCheck)
  DEFINES += MULTI_MEMCHECK=1
  linux:LIBS += -rdynamic
}
