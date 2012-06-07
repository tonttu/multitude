include(../multitude.pri)

HEADERS += AudioTransfer.hpp
HEADERS += Export.hpp
HEADERS += ShowGL.hpp
HEADERS += SubTitles.hpp
HEADERS += VideoDisplay.hpp
HEADERS += VideoInFFMPEG.hpp
HEADERS += VideoIn.hpp

SOURCES += AudioTransfer.cpp
SOURCES += ShowGL.cpp
SOURCES += SubTitles.cpp
SOURCES += VideoIn.cpp
SOURCES += VideoInFFMPEG.cpp

# New video player
HEADERS += AudioTransfer2.hpp AVDecoder.hpp AVDecoderFFMPEG.hpp MemoryPool.hpp
SOURCES += AudioTransfer2.cpp AVDecoder.cpp AVDecoderFFMPEG.cpp

LIBS += $$MULTI_FFMPEG_LIBS

DEFINES += __STDC_CONSTANT_MACROS

LIBS += $$LIB_RESONANT $$LIB_SCREENPLAY $$LIB_LUMINOUS $$LIB_NIMBLE
LIBS += $$LIB_RADIANT $$LIB_POETIC $$LIB_OPENGL $$LIB_RESONANT
LIBS += $$LIB_PATTERNS $$LIB_VALUABLE $$LIB_GLEW

macx:LIBS += -framework,OpenGL

DEFINES += VIDEODISPLAY_EXPORT

include(../library.pri)
