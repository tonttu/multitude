include(../multitude.pri)

HEADERS += ProgramGL.hpp
HEADERS += RenderDriverGL.hpp
HEADERS += ResourceHandleGL.hpp
HEADERS += StateGL.hpp
HEADERS += TextureGL.hpp
HEADERS += VertexArrayGL.hpp
HEADERS += BufferGL.hpp
HEADERS += Error.hpp
HEADERS += RenderTargetGL.hpp
HEADERS += PipelineCommand.hpp

HEADERS += FontCache.hpp
HEADERS += RichTextLayout.hpp
HEADERS += SimpleTextLayout.hpp
HEADERS += TextLayout.hpp

HEADERS += CocoaWindow.hpp
HEADERS += CodecRegistry.hpp
HEADERS += Collectable.hpp
HEADERS += ColorCorrection.hpp
HEADERS += ContextArray.hpp
HEADERS += ContextVariable.hpp
HEADERS += CPUMipmaps.hpp
HEADERS += DistanceFieldGenerator.hpp
HEADERS += DummyOpenGL.hpp
HEADERS += Export.hpp
HEADERS += GarbageCollector.hpp
HEADERS += GLKeyStone.hpp
HEADERS += GLResource.hpp
HEADERS += GLResources.hpp
HEADERS += GLSLProgramObject.hpp
HEADERS += GLSLShaderObject.hpp
HEADERS += Buffer.hpp
HEADERS += HardwareColorCorrection.hpp
HEADERS += ImageCodec.hpp
HEADERS += ImageCodecTGA.hpp
HEADERS += Image.hpp
HEADERS += Luminous.hpp
HEADERS += Mipmap.hpp
HEADERS += MultiHead.hpp
HEADERS += PixelFormat.hpp
HEADERS += PostProcess.hpp
HEADERS += PostProcessChain.hpp
HEADERS += PostProcessFilter.hpp
HEADERS += QtWindow.hpp
HEADERS += RenderContext.hpp
HEADERS += RenderContextImpl.hpp
HEADERS += RenderDriver.hpp
HEADERS += RenderManager.hpp
HEADERS += RenderResource.hpp
HEADERS += RenderTarget.hpp
HEADERS += ScreenDetector.hpp
HEADERS += Shader.hpp
HEADERS += ShaderUniform.hpp
HEADERS += Program.hpp
HEADERS += Spline.hpp
HEADERS += Style.hpp
HEADERS += Texture2.hpp
HEADERS += Texture.hpp
HEADERS += TextureAtlas.hpp
HEADERS += Transformer.hpp
HEADERS += UniformDescription.hpp
HEADERS += Utils.hpp
HEADERS += VertexArray.hpp
HEADERS += VertexAttribute.hpp
HEADERS += VertexBuffer.hpp
HEADERS += VertexBufferImpl.hpp
HEADERS += VertexDescription.hpp
HEADERS += RenderCommand.hpp
HEADERS += VM1.hpp
HEADERS += WindowEventHook.hpp
HEADERS += Window.hpp
linux-*:HEADERS += XRandR.hpp
linux-*:SOURCES += XRandR.cpp
!macx:HEADERS += ScreenDetectorAMD.hpp
!macx:HEADERS += ScreenDetectorNV.hpp
macx:OBJECTIVE_SOURCES += CocoaWindow.mm
!macx:SOURCES += ScreenDetectorAMD.cpp
!macx:SOURCES += ScreenDetectorNV.cpp
!mobile*:HEADERS += ImageCodecDDS.hpp
!mobile*:HEADERS += MipMapGenerator.hpp
!mobile*:HEADERS += SpriteRenderer.hpp
!mobile*:SOURCES += ImageCodecDDS.cpp
!mobile*:SOURCES += ImageCodecTGA.cpp
!mobile*:SOURCES += MipMapGenerator.cpp
!mobile*:SOURCES += SpriteRenderer.cpp

SOURCES += ProgramGL.cpp
SOURCES += RenderDriverGL.cpp
SOURCES += TextureGL.cpp
SOURCES += VertexArrayGL.cpp
SOURCES += BufferGL.cpp
SOURCES += Error.cpp
SOURCES += RenderTargetGL.cpp
SOURCES += PipelineCommand.cpp

SOURCES += FontCache.cpp
SOURCES += RichTextLayout.cpp
SOURCES += SimpleTextLayout.cpp
SOURCES += TextLayout.cpp

HEADERS += BlendMode.hpp
SOURCES += BlendMode.cpp
HEADERS += DepthMode.hpp
SOURCES += DepthMode.cpp
HEADERS += StencilMode.hpp
SOURCES += StencilMode.cpp

SOURCES += CodecRegistry.cpp
SOURCES += ColorCorrection.cpp
SOURCES += CPUMipmaps.cpp
SOURCES += DistanceFieldGenerator.cpp
SOURCES += GarbageCollector.cpp
SOURCES += GLKeyStone.cpp
SOURCES += GLResource.cpp
SOURCES += GLResources.cpp
SOURCES += GLSLProgramObject.cpp
SOURCES += GLSLShaderObject.cpp
SOURCES += Buffer.cpp
SOURCES += HardwareColorCorrection.cpp
SOURCES += Image.cpp
SOURCES += Luminous.cpp
SOURCES += Mipmap.cpp
SOURCES += MultiHead.cpp
SOURCES += PixelFormat.cpp
SOURCES += PostProcessChain.cpp
SOURCES += PostProcessFilter.cpp
SOURCES += QtWindow.cpp
SOURCES += RenderContext.cpp
SOURCES += RenderDriver.cpp
SOURCES += RenderManager.cpp
SOURCES += RenderResource.cpp
SOURCES += RenderTarget.cpp
SOURCES += ScreenDetector.cpp
SOURCES += Shader.cpp
SOURCES += Program.cpp
SOURCES += Spline.cpp
SOURCES += Texture2.cpp
SOURCES += Texture.cpp
SOURCES += TextureAtlas.cpp
SOURCES += Transformer.cpp
SOURCES += UniformDescription.cpp
SOURCES += Utils.cpp
SOURCES += VertexArray.cpp
SOURCES += VertexBuffer.cpp
SOURCES += VertexDescription.cpp
SOURCES += VM1.cpp
SOURCES += Window.cpp

# Link in Squish statically
LIBS += $$LIB_SQUISH
QMAKE_LIBDIR += ../Squish

LIBS += $$LIB_RADIANT \
    $$LIB_OPENGL \
    $$LIB_VALUABLE \
    $$LIB_NIMBLE \
    $$LIB_PATTERNS

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
linux-*:LIBS += -lXNVCtrl -lXrandr -lXext -lX11

INCLUDEPATH += ../3rdparty/adl_sdk

DEFINES += LUMINOUS_EXPORT

CONFIG += qt
QT += gui opengl

include(../library.pri)
