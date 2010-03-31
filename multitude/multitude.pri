# Use this to setup global build options & variables.
# This file is to be included by all project files of qmake.
CONFIG -= qt
CONFIG += link_pkgconfig
CONFIG += thread

CONFIG += embed_manifest_exe

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

MULTI_FFMPEG_LIBS = -lavcodec -lavutil -lavformat

withbundles = $$(MULTI_BUNDLES)

# Try to guess Win64
win32 {
    BITS=$$(PROCESSOR_ARCHITECTURE)
    contains(BITS,AMD64):CONFIG+=win64
}

LIB_POETIC = -lPoetic
LIB_FLUFFY = -lFluffy
LIB_LUMINOUS = -lLuminous
LIB_NIMBLE = -lNimble
LIB_OPENGL = -lGL -lGLU
LIB_GLU = -lGLU
LIB_RADIANT = -lRadiant -lPatterns
LIB_RESONANT = -lResonant
LIB_SCREENPLAY = -lScreenplay
LIB_VIDEODISPLAY = -lVideoDisplay
LIB_VALUABLE = -lValuable
LIB_PATTERNS = -lPatterns

MULTI_LIB_FLAG = -L

linux-*:LIB_GLEW = -lGLEW

macx {

  # withbundles = $$(MULTI_BUNDLES)
  withbundles = YES

  LIB_OPENGL = -framework,OpenGL
  LIB_GLU =
  LIB_GLEW = -lGLEW

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
  }

  # change architecture to x86_64 if snow leopard
  system([ `uname -r | cut -d . -f1` -gt 9 ] )  {
  CONFIG += x86_64
  }

}

win32 {
    win64:WINPORT_INCLUDE = $$PWD\Win64x\include
    else:WINPORT_INCLUDE = $$PWD\Win32x\include

    win64:INCLUDEPATH += $$PWD\Win64x\include
    else:INCLUDEPATH += $$PWD\Win32x\include

    win64:LIBPATH += $$PWD\Win64x\lib64
    else:LIBPATH += $$PWD\Win32x\lib32

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

