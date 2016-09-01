include(../multitude.pri)

CONFIG += qt
QT += gui

INCLUDEPATH += ../ThirdParty/adl_sdk

DEFINES += LUMINOUS_EXPORT

HEADERS += ProgramGL.hpp \
    CullMode.hpp \
    RenderDefines.hpp \
    RenderQueues.hpp \
    PostProcessContext.hpp \
    PostProcessFilter.hpp \
    GPUAssociation.hpp \
    MaskGuard.hpp \
    MipmapRenderer.hpp \
    ScreenDetectorQt.hpp \
    SwapGroups.hpp
HEADERS += RenderDriverGL.hpp
HEADERS += ResourceHandleGL.hpp
HEADERS += StateGL.hpp
HEADERS += TextureGL.hpp
HEADERS += VertexArrayGL.hpp
HEADERS += BufferGL.hpp
HEADERS += Error.hpp
HEADERS += FrameBufferGL.hpp
HEADERS += PipelineCommand.hpp

HEADERS += FontCache.hpp
HEADERS += RichTextLayout.hpp
HEADERS += SimpleTextLayout.hpp
HEADERS += TextLayout.hpp

HEADERS += CodecRegistry.hpp
HEADERS += ColorCorrection.hpp
HEADERS += RGBCube.hpp
HEADERS += ContextArray.hpp
HEADERS += DistanceFieldGenerator.hpp
HEADERS += Export.hpp
HEADERS += GLKeyStone.hpp
HEADERS += Buffer.hpp
HEADERS += HardwareColorCorrection.hpp
HEADERS += ImageCodec.hpp
HEADERS += ImageCodecTGA.hpp
HEADERS += Image.hpp
HEADERS += Luminous.hpp
HEADERS += Mipmap.hpp
HEADERS += MultiHead.hpp
HEADERS += PixelFormat.hpp
HEADERS += PostProcessChain.hpp
HEADERS += ColorCorrectionFilter.hpp
HEADERS += RenderContext.hpp
HEADERS += RenderContextImpl.hpp
HEADERS += RenderDriver.hpp
HEADERS += RenderManager.hpp
HEADERS += RenderResource.hpp
HEADERS += FrameBuffer.hpp
HEADERS += ScreenDetector.hpp
HEADERS += Shader.hpp
HEADERS += ShaderUniform.hpp
HEADERS += Program.hpp
HEADERS += Spline.hpp
HEADERS += Style.hpp
HEADERS += Texture.hpp
HEADERS += TextureAtlas.hpp
HEADERS += Transformer.hpp
HEADERS += UniformDescription.hpp
HEADERS += VertexArray.hpp
HEADERS += VertexAttribute.hpp
HEADERS += VertexDescription.hpp
HEADERS += RenderCommand.hpp
HEADERS += VM1.hpp
HEADERS += WindowEventHook.hpp
HEADERS += Window.hpp
HEADERS += ImageCodecDDS.hpp
HEADERS += ImageCodecCS.hpp
HEADERS += MipMapGenerator.hpp
HEADERS += SpriteRenderer.hpp
HEADERS += XRandR.hpp
HEADERS += Xinerama.hpp
HEADERS += SplineManager.hpp

SOURCES += ImageCodecDDS.cpp \
    GPUAssociation.cpp \
    MaskGuard.cpp \
    MipmapRenderer.cpp \
    ScreenDetectorQt.cpp \
    SwapGroups.cpp
SOURCES += ImageCodecCS.cpp
SOURCES += ImageCodecTGA.cpp
SOURCES += MipMapGenerator.cpp
SOURCES += SpriteRenderer.cpp

SOURCES += ProgramGL.cpp \
    CullMode.cpp \
    PostProcessContext.cpp \
    PostProcessFilter.cpp
SOURCES += RenderDriverGL.cpp
SOURCES += TextureGL.cpp
SOURCES += VertexArrayGL.cpp
SOURCES += BufferGL.cpp
SOURCES += Error.cpp
SOURCES += FrameBufferGL.cpp
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
SOURCES += RGBCube.cpp
SOURCES += DistanceFieldGenerator.cpp
SOURCES += GLKeyStone.cpp
SOURCES += Buffer.cpp
SOURCES += HardwareColorCorrection.cpp
SOURCES += Image.cpp
SOURCES += Luminous.cpp
SOURCES += Mipmap.cpp
SOURCES += MultiHead.cpp
SOURCES += PixelFormat.cpp
SOURCES += PostProcessChain.cpp
SOURCES += ColorCorrectionFilter.cpp
SOURCES += RenderContext.cpp
SOURCES += RenderDriver.cpp
SOURCES += RenderManager.cpp
SOURCES += RenderQueues.cpp
SOURCES += RenderResource.cpp
SOURCES += FrameBuffer.cpp
SOURCES += ScreenDetector.cpp
SOURCES += Shader.cpp
SOURCES += StateGL.cpp
SOURCES += Program.cpp
SOURCES += Spline.cpp
SOURCES += Texture.cpp
SOURCES += TextureAtlas.cpp
SOURCES += Transformer.cpp
SOURCES += UniformDescription.cpp
SOURCES += VertexArray.cpp
SOURCES += VertexDescription.cpp
SOURCES += VM1.cpp
SOURCES += Window.cpp
SOURCES += SplineManager.cpp

# Link in Squish statically
LIBS += $$LIB_SQUISH
QMAKE_LIBDIR += ../Squish

LIBS += $$LIB_RADIANT \
    $$LIB_OPENGL \
    $$LIB_VALUABLE \
    $$LIB_NIMBLE \
    $$LIB_PATTERNS

HEADERS += ImageCodecQT.hpp
HEADERS += ImageCodecSVG.hpp
SOURCES += ImageCodecQT.cpp
SOURCES += ImageCodecSVG.cpp
QT += gui
QT += svg

# Platform specific: Microsoft Windows
win32 {
  win64:LIBS += -lnvapi64
  else:LIBS += -lnvapi
  LIBS += -lUser32
}

# Platform specific: Apple OS X
macx {
} else {
  HEADERS += ScreenDetectorAMD.hpp
  HEADERS += ScreenDetectorNV.hpp
  SOURCES += ScreenDetectorAMD.cpp
  SOURCES += ScreenDetectorNV.cpp
}

# Platform specific: GNU Linux
linux-* {
  LIBS += -lXNVCtrl -lXrandr -lXext -lX11 -lXinerama

  SOURCES += XRandR.cpp
  SOURCES += Xinerama.cpp

  QT += x11extras
}

enable-pdf {
  linux-* {
    INCLUDEPATH += /opt/multitouch-pdfium1/include
    QMAKE_LIBDIR += /opt/multitouch-pdfium1/lib
    LIBS += -Wl,-whole-archive -lmultitouch-pdfium1 -Wl,-no-whole-archive
  }

  macx {
    PKGCONFIG += multitouch-pdfium1
  }

  HEADERS += PDFManager.hpp
  SOURCES += PDFManager.cpp
}
include(../library.pri)
