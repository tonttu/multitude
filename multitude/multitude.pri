# Use this to setup global build options & variables.
# This file is to be included by all project files of qmake.
CONFIG -= qt
CONFIG += link_pkgconfig
CONFIG += thread

CONFIG += embed_manifest_exe

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

# The Cornerstone version for libraries
unix {
  MULTITUDE_VERSION_MAJOR=$$system(cat ../VERSION | cut -d . -f 1)
  MULTITUDE_VERSION_MINOR=$$system(cat ../VERSION | cut -d . -f 2)
  MULTITUDE_VERSION_PATCH=$$system(cat ../VERSION | cut -d . -f 3 | cut -d - -f 1)

  VERSION = $${MULTITUDE_VERSION_MAJOR}.$${MULTITUDE_VERSION_MINOR}.$${MULTITUDE_VERSION_PATCH}
}

withbundles = $$(MULTI_BUNDLES)

MULTI_FFMPEG_LIBS = -lavcodec -lavformat -lavutil

LIB_BOX2D = -lBox2D

!mobile {
  # exists(/usr/local/lib/libftd2xx.so)|exists(/opt/multitouch/lib/libftd2xx.dylib) {
  # message(FTD2XX support detected.)
  CONFIG += with-ftd2xx
  with-ftd2xx {
    LIB_FTD2XX = -lftd2xx
    WITH_FTD2XX = yes
    DEFINES += MULTI_WITH_FTD2XX=1
  }
}


LIB_OPENCL = -lOpenCL
LIB_OPENGL = -lGL -lGLU

LIB_POETIC = -lPoetic
LIB_FLUFFY = -lFluffy
LIB_LUMINOUS = -lLuminous
LIB_NIMBLE = -lNimble
LIB_RADIANT = -lRadiant
LIB_RESONANT = -lResonant
LIB_SCREENPLAY = -lScreenplay
LIB_VIDEODISPLAY = -lVideoDisplay
LIB_VALUABLE = -lValuable
LIB_PATTERNS = -lPatterns
LIB_SQUISH = -lSquish

linux-*:vivid {
  QMAKE_LIBDIR += $$(FBX_SDK)/lib/gcc4
  LIB_VIVID = -lVivid -lfbxsdk_20113_1_x64
}

linux-*{
  contains(USEGLEW,no) {
    DEFINES += MULTI_WITHOUT_GLEW=1
  } else {
    LIB_GLEW=-lGLEW
  }

  QMAKE_LIBDIR += /usr/lib/nvidia-current

  !mobile:QMAKE_LIBDIR += $$PWD/Linux/lib

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

macx {
  # For Deft (which depends on MultiTouch)
  LIBS += -undefined dynamic_lookup

  # Frameworks on OS X don't respect QMAKE_LIBDIR
  !mobile:QMAKE_LFLAGS += -F$$PWD/lib -L$$PWD/OSX/lib

  # withbundles = $$(MULTI_BUNDLES)
  withbundles = YES

  LIB_OPENCL = -framework,OpenCL
  LIB_OPENGL = -framework,OpenGL
  # LIB_GLEW = -lGLEW

  DEFINES += QT_MAC_USE_COCOA Q_OS_MAC64

  # DEFINES += __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=1050

  contains(withbundles,YES) {

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
  }

  # change architecture to x86_64 if snow leopard
  system([ `uname -r | cut -d . -f1` -gt 9 ] )  {
  CONFIG += x86_64

  system([ `uname -r | cut -d . -f1` -eq 10 ] ):DEFINES+=RADIANT_OSX_SNOW_LEOPARD
  system([ `uname -r | cut -d . -f1` -eq 11 ] ):DEFINES+=RADIANT_OSX_LION
  }

}

win32 {
    # Try to identify used compiler on Windows (32 vs 64)
    COMPILER_OUTPUT=$$system(cl 2>&1)
    contains(COMPILER_OUTPUT,x64):CONFIG+=win64

    win64:WINPORT_INCLUDE = $$PWD\\Win64x\\include
    else:WINPORT_INCLUDE = $$PWD\\Win32x\\include

    win64:INCLUDEPATH += $$PWD\\Win64x\\include
    else:INCLUDEPATH += $$PWD\\Win32x\\include

    win64:QMAKE_LIBDIR += $$PWD\\Win64x\\lib64
    else:QMAKE_LIBDIR += $$PWD\\Win32x\\lib32

    win64:LIB_GLEW = -lglew64
    else:LIB_GLEW = -lglew32

    DDK_PATH="C:\\WinDDK\\7600.16385.1"

    LIB_OPENGL = -lopengl32 -lglu32
    QMAKE_CXXFLAGS += -D_CRT_SECURE_NO_WARNINGS -wd4244 -wd4251 -wd4355
    DEFINES += WIN32

    # These libs have an extra extension for debug builds
    build_pass:CONFIG(debug,debug|release) {
      LIB_BOX2D = -lBox2D_d
      LIB_POETIC = -lPoetic_d
      LIB_FLUFFY = -lFluffy_d
      LIB_LUMINOUS = -lLuminous_d
      LIB_NIMBLE = -lNimble_d
      LIB_RADIANT = -lRadiant_d
      LIB_RESONANT = -lResonant_d
      LIB_SCREENPLAY = -lScreenplay_d
      LIB_VIDEODISPLAY = -lVideoDisplay_d
      LIB_VALUABLE = -lValuable_d
      LIB_PATTERNS = -lPatterns_d
      LIB_SQUISH = -lSquish_d
    }
}

MULTI_VIDEO_LIBS = $$LIB_SCREENPLAY $$LIB_RESONANT $$LIB_VIDEODISPLAY

QMAKE_LIBDIR += $$PWD/lib

# Disable asserts in release mode
build_pass:CONFIG(release, debug|release) {
  DEFINES += NDEBUG
}

# Use ccache if available
unix:exists(/usr/bin/ccache):QMAKE_CXX=ccache g++
unix:exists(/sw/bin/ccache):QMAKE_CXX=/sw/bin/ccache g++
unix:exists(/opt/local/bin/ccache):QMAKE_CXX=/opt/local/bin/ccache g++

unix:exists(/opt/multitouch):INCLUDEPATH+=/opt/multitouch/include
unix:exists(/opt/multitouch):LIBS+=-L/opt/multitouch/lib
