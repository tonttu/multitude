TEMPLATE = lib

CONFIG += shared

win32 {
  DEFINES += WIN32 WIN32_MEAN_AND_LEAN VC_EXTRALEAN GLEW_BUILD
  LIBS += opengl32.lib
}

TARGET = glew

SOURCES += src/glew.c

INCLUDEPATH += include

DESTDIR = ../../lib

include(../../library.pri)
