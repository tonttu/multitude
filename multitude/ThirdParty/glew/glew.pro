TEMPLATE = lib

CONFIG += shared

win32 {
  DEFINES += WIN32 WIN32_MEAN_AND_LEAN VC_EXTRALEAN GLEW_BUILD
  LIBS += opengl32.lib
}

TARGET = glew

SOURCES += src/glew.c

HEADERS += include/GL/glew.h
HEADERS += include/GL/glxew.h
HEADERS += include/GL/wglew.h

INCLUDEPATH += include

DESTDIR = ../../lib

include(../../library.pri)

# Override default installation
src_code.path = /src/multitude/ThirdParty/glew/src
src_code.files = $$SOURCES

src_headers.path = /src/multitude/ThirdParty/glew/include/GL
src_headers.files = $$HEADERS

src_projectfile.path = /src/multitude/ThirdParty/glew
src_projectfile.files = $$PROJECT_FILE

includes.path = /include/GL

INSTALLS += src_headers src_projectfile
