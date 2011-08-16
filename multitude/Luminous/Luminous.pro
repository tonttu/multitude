include(../multitude.pri)

HEADERS += BGThread.hpp
HEADERS += CodecRegistry.hpp
HEADERS += Collectable.hpp
HEADERS += ContextVariable.hpp
HEADERS += CPUMipmaps.hpp
HEADERS += CPUMipmapStore.hpp
HEADERS += EnableStep.hpp
HEADERS += Error.hpp
HEADERS += Export.hpp
HEADERS += FramebufferObject.hpp
HEADERS += FramebufferResource.hpp
HEADERS += GarbageCollector.hpp
HEADERS += GLContext.hpp
HEADERS += GLKeyStone.hpp
HEADERS += GLResource.hpp
HEADERS += GLResources.hpp
HEADERS += GLSLProgramObject.hpp
HEADERS += GLSLShaderObject.hpp
HEADERS += ImageCodecDDS.hpp
HEADERS += ImageCodec.hpp
HEADERS += ImageCodecQT.hpp
HEADERS += ImageCodecSVG.hpp
HEADERS += ImageCodecTGA.hpp
HEADERS += Image.hpp
HEADERS += Luminous.hpp
HEADERS += MatrixStep.hpp
HEADERS += MipMapGenerator.hpp
HEADERS += MultiHead.hpp
HEADERS += PixelFormat.hpp
HEADERS += RenderContext.hpp
#HEADERS += RenderTarget.hpp
HEADERS += Shader.hpp
HEADERS += SpriteRenderer.hpp
HEADERS += Task.hpp
HEADERS += Texture.hpp
HEADERS += Transformer.hpp
HEADERS += Utils.hpp
HEADERS += VertexBuffer.hpp
HEADERS += VertexBufferImpl.hpp

SOURCES += BGThread.cpp
SOURCES += CodecRegistry.cpp
SOURCES += CPUMipmaps.cpp
SOURCES += CPUMipmapStore.cpp
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
SOURCES += ImageCodecDDS.cpp
SOURCES += ImageCodecQT.cpp
SOURCES += ImageCodecSVG.cpp
SOURCES += ImageCodecTGA.cpp
SOURCES += Image.cpp
SOURCES += Luminous.cpp
SOURCES += MipMapGenerator.cpp
SOURCES += MultiHead.cpp
SOURCES += PixelFormat.cpp
SOURCES += RenderContext.cpp
#SOURCES += RenderTarget.cpp
SOURCES += Shader.cpp
SOURCES += SpriteRenderer.cpp
SOURCES += Task.cpp
SOURCES += Texture.cpp
SOURCES += Transformer.cpp
SOURCES += Utils.cpp
SOURCES += VertexBuffer.cpp

# Link in Squish statically
LIBS += -lSquish
QMAKE_LIBDIR += ../Squish

LIBS += $$LIB_RADIANT \
    $$LIB_OPENGL \
    $$LIB_VALUABLE \
    $$LIB_NIMBLE \
    $$LIB_PATTERNS \
    $$LIB_GLEW

win32:DEFINES += LUMINOUS_EXPORT

CONFIG += qt
QT += gui
QT += svg

# On Windows we need to install the Qt plugins
win32 {
  qt_plugin_install.path += /bin
  qt_plugin_install.files = $$[QT_INSTALL_PLUGINS]
  INSTALLS += qt_plugin_install
}

include(../library.pri)
