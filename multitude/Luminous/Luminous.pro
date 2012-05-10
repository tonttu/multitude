include(../multitude.pri)

# Drivers
HEADERS += RenderDriver.hpp
HEADERS += RenderDriverGL.hpp
SOURCES += RenderDriverGL.cpp

# Device contexts
HEADERS += GLContext.hpp
SOURCES += GLContext.cpp

# Render contexts
HEADERS += RenderContext2.hpp
HEADERS += RenderContextImmediate.hpp
SOURCES += RenderContextImmediate.cpp

# Render resources
HEADERS += RenderResource.hpp
HEADERS += HardwareBuffer.hpp
SOURCES += HardwareBuffer.cpp
HEADERS += VertexAttributeBinding.hpp
SOURCES += VertexAttributeBinding.cpp
HEADERS += ShaderConstantBlock.hpp
SOURCES += ShaderConstantBlock.cpp

HEADERS += VertexDescription.hpp
SOURCES += VertexDescription.cpp

# Shaders
HEADERS += ShaderConstant.hpp

# Utility functions
HEADERS += GLUtils.hpp
SOURCES += GLUtils.cpp

HEADERS += BGThread.hpp
linux-*:HEADERS += XRandR.hpp
!macx:HEADERS += ScreenDetectorAMD.hpp
HEADERS += ScreenDetector.hpp
!macx:HEADERS += ScreenDetectorNV.hpp
HEADERS += HardwareColorCorrection.hpp
HEADERS += VM1.hpp
HEADERS += VertexHolder.hpp
HEADERS += Style.hpp
HEADERS += DummyOpenGL.hpp
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

SOURCES += Style.cpp
SOURCES += VertexHolder.cpp
SOURCES += BGThread.cpp
linux-*:SOURCES += XRandR.cpp
!macx:SOURCES += ScreenDetectorAMD.cpp
SOURCES += ScreenDetector.cpp
!macx:SOURCES += ScreenDetectorNV.cpp
SOURCES += HardwareColorCorrection.cpp
HEADERS += ColorCorrection.hpp
SOURCES += CodecRegistry.cpp
SOURCES += CPUMipmaps.cpp
SOURCES += Error.cpp
SOURCES += FramebufferObject.cpp
SOURCES += FramebufferResource.cpp
SOURCES += GarbageCollector.cpp

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
SOURCES += ColorCorrection.cpp
SOURCES += VM1.cpp

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

win32 {
  win64:LIBS += -lnvapi64
  else:LIBS += -lnvapi
  LIBS += -lUser32
}
linux-*:LIBS += -lXNVCtrl -lXrandr

INCLUDEPATH += ../Externals/adl_sdk

DEFINES += LUMINOUS_EXPORT

CONFIG += qt
QT += gui
# QT += svg

include(../library.pri)


