TEMPLATE = lib

CONFIG += staticlib

win32 {
  build_pass:CONFIG(debug,debug|release) {
    DEFINES += WIN32 _DEBUG _LIB WIN32_LEAN_AND_MEAN VC_EXTRALEAN GLEW_STATIC
  } else {
    DEFINES = WIN32 NDEBUG _LIB WIN32_LEAN_AND_MEAN VC_EXTRALEAN GLEW_STATIC
  }
}

TARGET = glew

SOURCES += ../glew-1.8.0/src/glew.c

INCLUDEPATH += ../glew-1.8.0/include

DESTDIR = ../lib