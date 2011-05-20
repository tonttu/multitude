include(../multitude.pri)
HEADERS += BGThread.hpp \
    GLContext.hpp
HEADERS += FramebufferResource.hpp
HEADERS += CodecRegistry.hpp
HEADERS += Collectable.hpp
HEADERS += ContextVariable.hpp
HEADERS += CPUMipmaps.hpp
HEADERS += CPUMipmapStore.hpp
HEADERS += EnableStep.hpp
HEADERS += Error.hpp
HEADERS += Export.hpp
HEADERS += FramebufferObject.hpp
HEADERS += GarbageCollector.hpp
HEADERS += GLKeyStone.hpp
HEADERS += GLResource.hpp
HEADERS += GLResources.hpp
HEADERS += GLSLProgramObject.hpp
HEADERS += GLSLShaderObject.hpp
HEADERS += GPUMipmaps.hpp
HEADERS += ImageCodec.hpp
HEADERS += ImageCodecSVG.hpp
HEADERS += ImageCodecTGA.hpp
HEADERS += Image.hpp
HEADERS += Luminous.hpp
HEADERS += MatrixStep.hpp
HEADERS += MultiHead.hpp
HEADERS += PixelFormat.hpp
HEADERS += RenderContext.hpp
HEADERS += Shader.hpp
HEADERS += Task.hpp
HEADERS += Texture.hpp
HEADERS += Transformer.hpp
HEADERS += Utils.hpp
HEADERS += VertexBuffer.hpp
HEADERS += VertexBufferImpl.hpp
HEADERS += ColorCorrection.hpp

SOURCES += BGThread.cpp \
    GLContext.cpp
SOURCES += FramebufferResource.cpp
SOURCES += CodecRegistry.cpp
SOURCES += Collectable.cpp
SOURCES += CPUMipmaps.cpp
SOURCES += CPUMipmapStore.cpp
SOURCES += Error.cpp
SOURCES += FramebufferObject.cpp
SOURCES += GarbageCollector.cpp
SOURCES += GLKeyStone.cpp
SOURCES += GLResource.cpp
SOURCES += GLResources.cpp
SOURCES += GLSLProgramObject.cpp
SOURCES += GLSLShaderObject.cpp
SOURCES += GPUMipmaps.cpp
SOURCES += ImageCodecSVG.cpp
SOURCES += ImageCodecTGA.cpp
SOURCES += Image.cpp
SOURCES += Luminous.cpp
SOURCES += MultiHead.cpp
SOURCES += PixelFormat.cpp
SOURCES += RenderContext.cpp
SOURCES += Shader.cpp
SOURCES += Task.cpp
SOURCES += Texture.cpp
SOURCES += Transformer.cpp
SOURCES += Utils.cpp
SOURCES += VertexBuffer.cpp
SOURCES += ColorCorrection.cpp

LIBS += $$LIB_RADIANT \
    $$LIB_OPENGL \
    $$LIB_VALUABLE \
    $$LIB_GLU \
    $$LIB_NIMBLE \
    $$LIB_PATTERNS \
    $$LIB_GLEW
unix:!contains(HAS_QT_45,YES) { 
    HEADERS += ImageCodecPNG.hpp
    HEADERS += ImageCodecTGA.hpp
    SOURCES += ImageCodecJPEG.cpp
    SOURCES += ImageCodecPNG.cpp
    LIBS += -ljpeg \
        -lpng
}
win32:DEFINES += LUMINOUS_EXPORT
contains(HAS_QT_45,YES) { 
    message(Including QT Image codecs)
    HEADERS += ImageCodecQT.hpp
    SOURCES += ImageCodecQT.cpp
    CONFIG += qt
    QT += gui
    QT += svg
    # On Windows we need to install the Qt plugins
    win32 {
      qt_plugin_install.path += /bin
      qt_plugin_install.files = $$[QT_INSTALL_PLUGINS]
      INSTALLS += qt_plugin_install
    }
}
include(../library.pri)
