include(../../cornerstone.pri)

HEADERS += Export.hpp

HEADERS += SubTitles.hpp
HEADERS += VideoDisplay.hpp

SOURCES += SubTitles.cpp

HEADERS += AudioTransfer.hpp AVDecoder.hpp Utils.hpp
SOURCES += AudioTransfer.cpp AVDecoder.cpp

HEADERS += FfmpegDecoder.hpp
SOURCES += FfmpegDecoder.cpp

win32 {
  QMAKE_LIBDIR += $$CORNERSTONE_DEPS_PATH/ffmpeg/bin
}

DEFINES += __STDC_CONSTANT_MACROS

LIBS += $$LIB_RESONANT $$LIB_SCREENPLAY $$LIB_LUMINOUS $$LIB_NIMBLE
LIBS += $$LIB_RADIANT $$LIB_OPENGL $$LIB_RESONANT
LIBS += $$LIB_PATTERNS $$LIB_VALUABLE $$LIB_FFMPEG

# TODO: Should update our code that usese deprecated FFMPEG API
*clang* | *g++*: QMAKE_CXXFLAGS_WARN_ON += -Wno-error=deprecated-declarations

macx {
  LIBS += -framework OpenGL
  PKGCONFIG += libavdevice
}

DEFINES += VIDEODISPLAY_EXPORT

include(../library.pri)
