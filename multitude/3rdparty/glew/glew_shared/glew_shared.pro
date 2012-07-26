TEMPLATE = lib

CONFIG += shared

win32 {
  build_pass:CONFIG(debug,debug|release) {
    DEFINES += WIN32 WIN32_MEAN_AND_LEAN VC_EXTRALEAN GLEW_BUILD
  } else {
    DEFINES = WIN32 WIN32_LEAN_AND_MEAN VC_EXTRALEAN GLEW_BUILD
  }
  LIBS += opengl32.lib
}

TARGET = glew

SOURCES += ../glew-1.8.0/src/glew.c

INCLUDEPATH += ../glew-1.8.0/include

DESTDIR = ../lib