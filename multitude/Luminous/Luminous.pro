include(../multitude.pri)

HEADERS += BGThread.hpp \
    DummyOpenGL.hpp \
    Style.hpp \
    VertexHolder.hpp
HEADERS += FramebufferResource.hpp
HEADERS += CodecRegistry.hpp
HEADERS += Collectable.hpp
HEADERS += ContextVariable.hpp
HEADERS += CPUMipmaps.hpp
HEADERS += EnableStep.hpp
HEADERS += Error.hpp
HEADERS += Export.hpp
HEADERS += FramebufferObject.hpp
HEADERS += GarbageCollector.hpp
HEADERS += GLContext.hpp
HEADERS += GLKeyStone.hpp
HEADERS += GLResource.hpp
HEADERS += GLResources.hpp
HEADERS += GLSLProgramObject.hpp
HEADERS += GLSLShaderObject.hpp
!mobile*:HEADERS += ImageCodecDDS.hpp
HEADERS += ImageCodec.hpp
HEADERS += ImageCodecTGA.hpp
HEADERS += Image.hpp
HEADERS += Luminous.hpp
HEADERS += MatrixStep.hpp
!mobile*:HEADERS += MipMapGenerator.hpp
HEADERS += MultiHead.hpp
HEADERS += PixelFormat.hpp
HEADERS += RenderContext.hpp
#HEADERS += RenderTarget.hpp
HEADERS += Shader.hpp
!mobile*:HEADERS += SpriteRenderer.hpp
HEADERS += Task.hpp
HEADERS += Texture.hpp
HEADERS += Transformer.hpp
HEADERS += Utils.hpp
HEADERS += VertexBuffer.hpp
HEADERS += VertexBufferImpl.hpp
SOURCES += BGThread.cpp \
    Style.cpp \
    VertexHolder.cpp
SOURCES += CodecRegistry.cpp
SOURCES += CPUMipmaps.cpp
SOURCES += Error.cpp
SOURCES += FramebufferObject.cpp
SOURCES += FramebufferResource.cpp
SOURCES += GarbageCollector.cpp
SOURCES += GLContext.cpp
SOURCES += GLKeyStone.cpp
SOURCES += GLResource.cpp
SOURCES += GLResources.cpp
SOURCES += GLSLProgramObject.cpp
SOURCES += GLSLShaderObject.cpp
# TGA loader tries to create BGR & BGRA textures, which are not availale on OpenGL ES
!mobile*:SOURCES += ImageCodecTGA.cpp
!mobile*:SOURCES += ImageCodecDDS.cpp
SOURCES += Image.cpp
SOURCES += Luminous.cpp
!mobile*:SOURCES += MipMapGenerator.cpp
SOURCES += MultiHead.cpp
SOURCES += PixelFormat.cpp
SOURCES += RenderContext.cpp
#SOURCES += RenderTarget.cpp
SOURCES += Shader.cpp
!mobile*:SOURCES += SpriteRenderer.cpp
SOURCES += Task.cpp
SOURCES += Texture.cpp
SOURCES += Transformer.cpp
SOURCES += Utils.cpp
SOURCES += VertexBuffer.cpp

# Link in Squish statically
LIBS += $$LIB_SQUISH
QMAKE_LIBDIR += ../Squish

LIBS += $$LIB_RADIANT \
    $$LIB_OPENGL \
    $$LIB_VALUABLE \
    $$LIB_NIMBLE \
    $$LIB_PATTERNS \
    $$LIB_GLEW

DEFINES += LUMINOUS_COMPILE
# mobile*:HAS_QT_45 = NO
unix:!contains(HAS_QT_45,YES) {
    HEADERS += ImageCodecJPEG.hpp
    HEADERS += ImageCodecPNG.hpp
    SOURCES += ImageCodecJPEG.cpp
    SOURCES += ImageCodecPNG.cpp
    LIBS += -ljpeg \
        -lpng
}
win32:DEFINES += LUMINOUS_EXPORT
contains(HAS_QT_45,YES) {
    message(Including QT Image codecs)
    HEADERS += ImageCodecQT.hpp
    !mobile*:HEADERS += ImageCodecSVG.hpp
    SOURCES += ImageCodecQT.cpp
    !mobile*:SOURCES += ImageCodecSVG.cpp
    CONFIG += qt
    QT += gui
    !mobile*:QT += svg

    # On Windows we need to install the Qt plugins
    win32 {
        qt_plugin_install.path += /bin
        qt_plugin_install.files = $$[QT_INSTALL_PLUGINS]
        INSTALLS += qt_plugin_install
    }
}
DEFINES += LUMINOUS_EXPORT

CONFIG += qt
QT += gui
# QT += svg

include(../library.pri)


