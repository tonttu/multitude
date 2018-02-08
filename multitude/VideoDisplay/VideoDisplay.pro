include(../../cornerstone.pri)

HEADERS += Export.hpp \
    VideoCaptureMonitor.hpp

HEADERS += SubTitles.hpp
HEADERS += VideoDisplay.hpp

SOURCES += SubTitles.cpp

HEADERS += AudioTransfer.hpp AVDecoder.hpp Utils.hpp
SOURCES += AudioTransfer.cpp AVDecoder.cpp

HEADERS += FfmpegDecoder.hpp
SOURCES += FfmpegDecoder.cpp

HEADERS += FfmpegVideoFormatSelector.hpp
SOURCES += FfmpegVideoFormatSelector.cpp

win32 {
  SOURCES += WindowsVideoMonitor.cpp \
             WindowsVideoHelpers.cpp \
             RGBEasy.cpp \
             FfmpegVideoFormatSelectorWin.cpp

  HEADERS += WindowsVideoHelpers.hpp \
             RGBEasy.hpp

  QMAKE_LIBDIR += $$CORNERSTONE_DEPS_PATH/ffmpeg/bin

  # RGBEASY (Datapath SDK)
  INCLUDEPATH += rgbeasy-sdk-v7.14.1/include
  LIBS += -lStrmiids -lOle32 -lOleAut32 -lPropsys
}

linux {
  SOURCES += FfmpegVideoFormatSelectorLinux.cpp
  SOURCES += V4L2Monitor.cpp
}

macx {
  SOURCES += FfmpegVideoFormatSelectorDummy.cpp
  SOURCES += VideoCaptureMonitorDummy.cpp
}

DEFINES += __STDC_CONSTANT_MACROS

LIBS += $$LIB_RESONANT $$LIB_SCREENPLAY $$LIB_LUMINOUS $$LIB_NIMBLE
LIBS += $$LIB_RADIANT $$LIB_OPENGL $$LIB_RESONANT
LIBS += $$LIB_PATTERNS $$LIB_VALUABLE $$LIB_FFMPEG

# TODO: Should update our code that uses deprecated FFMPEG API
*clang* | *g++*: QMAKE_CXXFLAGS_WARN_ON += -Wno-error=deprecated-declarations

# TODO: Should handle errors
*clang* | *g++*: QMAKE_CXXFLAGS_WARN_ON += -Wno-error=unused-result

macx {
  LIBS += -framework OpenGL
  PKGCONFIG += libavdevice
}

DEFINES += VIDEODISPLAY_EXPORT

include(../../library.pri)
