# Use this to setup global build options & variables.
# This file is to be included by all project files of qmake.
CONFIG -= qt
CONFIG += link_pkgconfig
CONFIG += thread

CONFIG += embed_manifest_exe

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

# The Cornerstone version for libraries
unix:VERSION = 1.2.0

withbundles = $$(MULTI_BUNDLES)

MULTI_FFMPEG_LIBS = -lavcodec -lavutil -lavformat

LIB_POETIC = -lPoetic
LIB_FLUFFY = -lFluffy
LIB_LUMINOUS = -lLuminous
LIB_NIMBLE = -lNimble
LIB_OPENCL = -lOpenCL
LIB_OPENGL = -lGL -lGLU
LIB_GLU = -lGLU
LIB_RADIANT = -lRadiant -lPatterns
LIB_RESONANT = -lResonant
LIB_SCREENPLAY = -lScreenplay
LIB_VIDEODISPLAY = -lVideoDisplay
LIB_VALUABLE = -lValuable
LIB_PATTERNS = -lPatterns

linux-*:vivid {
  QMAKE_LIBDIR += $$(FBX_SDK)/lib/gcc4
  LIB_VIVID = -lVivid -lfbxsdk_20113_1_x64
}

LIB_BOX2D = -lBox2D

MULTI_LIB_FLAG = -L

linux-*{
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

  contains(MEMCHECK,yes) {
    DEFINES += MULTI_MEMCHECK=1
  }
}

macx {
  # withbundles = $$(MULTI_BUNDLES)
  withbundles = YES

  LIB_OPENCL = -framework,OpenCL
  LIB_OPENGL = -framework,OpenGL
  LIB_GLU =
  # LIB_GLEW = -lGLEW
  LIBS += -L$$PWD/lib

DEFINES += QT_MAC_USE_COCOA Q_OS_MAC64

  # DEFINES += __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=1050

  contains(withbundles,YES) {

    MULTI_LIB_FLAG = -F

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

    LIB_BOX2D = -framework,Box2D
  }

  # change architecture to x86_64 if snow leopard
  system([ `uname -r | cut -d . -f1` -gt 9 ] )  {
  CONFIG += x86_64
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

    LIB_OPENGL = -lopengl32
    LIB_GLU = -lglu32
    QMAKE_CXXFLAGS += -D_CRT_SECURE_NO_WARNINGS -wd4244 -wd4251 -wd4355
    DEFINES += WIN32

}

MULTI_VIDEO_LIBS = $$LIB_SCREENPLAY $$LIB_RESONANT $$LIB_VIDEODISPLAY

LIBS += $${MULTI_LIB_FLAG}$$PWD/lib

# message(QT version is $${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION}.$${QT_PATCH_VERSION})

contains(QT_MAJOR_VERSION,4) {

  contains(QT_MINOR_VERSION,5) || contains(QT_MINOR_VERSION,6) {
    HAS_QT_45=YES
    DEFINES += USE_QT45
  }

}

# Disable asserts in release mode
CONFIG(release, debug|release) {
  DEFINES += NDEBUG
}

# Use ccache if available
unix:exists(/usr/bin/ccache):QMAKE_CXX=ccache g++
unix:exists(/sw/bin/ccache):QMAKE_CXX=/sw/bin/ccache g++
unix:exists(/opt/local/bin/ccache):QMAKE_CXX=/opt/local/bin/ccache g++

unix:exists(/opt/multitouch):INCLUDEPATH+=/opt/multitouch/include
unix:exists(/opt/multitouch):LIBS+=-L/opt/multitouch/lib
