include(../multitude.pri)

HEADERS += Export.hpp

HEADERS += SubTitles.hpp
HEADERS += VideoDisplay.hpp

SOURCES += SubTitles.cpp

HEADERS += AudioTransfer.hpp AVDecoder.hpp Utils.hpp
SOURCES += AudioTransfer.cpp AVDecoder.cpp

TEST=$$(USE_FFMPEG)
isEmpty(TEST) {
  HEADERS += LibavDecoder.hpp
  SOURCES += LibavDecoder.cpp
} else {
  HEADERS += FfmpegDecoder.hpp
  SOURCES += FfmpegDecoder.cpp
}

LIBS += $$MULTI_FFMPEG_LIBS

DEFINES += __STDC_CONSTANT_MACROS

LIBS += $$LIB_RESONANT $$LIB_SCREENPLAY $$LIB_LUMINOUS $$LIB_NIMBLE
LIBS += $$LIB_RADIANT $$LIB_OPENGL $$LIB_RESONANT
LIBS += $$LIB_PATTERNS $$LIB_VALUABLE

macx {
  LIBS += -framework,OpenGL
  PKGCONFIG += libavdevice
}

DEFINES += VIDEODISPLAY_EXPORT

include(../library.pri)
