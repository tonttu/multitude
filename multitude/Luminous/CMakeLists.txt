include(../../common.cmake)

project(Luminous VERSION ${CORNERSTONE_VERSION} DESCRIPTION "A library of C++ classes for computer graphics using OpenGL")

add_library(Luminous SHARED)

set(SOURCES
  ImageCodecDDS.cpp
  GPUAssociation.cpp
  MaskGuard.cpp
  MipmapRenderer.cpp
  ScreenDetectorQt.cpp
  SwapGroups.cpp
  MemoryManager.cpp
  ImageCodecCS.cpp
  ImageCodecTGA.cpp
  MipMapGenerator.cpp
  SpriteRenderer.cpp
  ProgramGL.cpp
  CullMode.cpp
  PostProcessContext.cpp
  PostProcessFilter.cpp
  RenderDriverGL.cpp
  TextureGL.cpp
  VertexArrayGL.cpp
  BufferGL.cpp
  Error.cpp
  FrameBufferGL.cpp
  PipelineCommand.cpp
  FontCache.cpp
  RichTextLayout.cpp
  SimpleTextLayout.cpp
  TextLayout.cpp
  BlendMode.cpp
  DepthMode.cpp
  StencilMode.cpp
  CodecRegistry.cpp
  ColorCorrection.cpp
  RGBCube.cpp
  DistanceFieldGenerator.cpp
  GLKeyStone.cpp
  Buffer.cpp
  Image.cpp
  Luminous.cpp
  Mipmap.cpp
  MultiHead.cpp
  PixelFormat.cpp
  PostProcessChain.cpp
  ColorCorrectionFilter.cpp
  RenderContext.cpp
  RenderDriver.cpp
  RenderManager.cpp
  RenderResource.cpp
  FrameBuffer.cpp
  ScreenDetector.cpp
  Shader.cpp
  StateGL.cpp
  Program.cpp
  Spline.cpp
  Texture.cpp
  TextureAtlas.cpp
  Transformer.cpp
  UniformDescription.cpp
  VertexArray.cpp
  VertexDescription.cpp
  Window.cpp
  SplineManager.cpp
  BezierSplineFitter.cpp
  BezierSplineBuilder.cpp
  BezierSplineTesselator.cpp
  BezierSplineRenderer.cpp
  BezierSpline.cpp
  ImageCodecQT.cpp
  ImageCodecSVG.cpp
  ScreenDetectorAMD.cpp
  ScreenDetectorNV.cpp
  XRandR.cpp
  Xinerama.cpp
)

if(UNIX)

  if(ENABLE_PDF)
    target_include_directories(Radiant PRIVATE /opt/multitaction-pdfium2/include)
    target_link_libraries(Luminous PRIVATE /opt/multitaction-pdfium2/lib/libmultitaction-pdfium2.so)
  endif()

  find_package(Qt5 COMPONENTS X11Extras PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
  target_link_libraries(Luminous PRIVATE Qt5::X11Extras)
  target_link_libraries(Luminous PRIVATE XNVCtrl Xrandr Xext Xinerama)

endif()

set_target_properties(Luminous PROPERTIES SOURCES "${SOURCES}")

add_definitions(-DLUMINOUS_EXPORT)

find_package(Qt5 COMPONENTS Core PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Network PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Gui PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Svg PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

target_link_libraries(Luminous PRIVATE Squish)
target_link_libraries(Luminous PUBLIC Valuable Nimble)
target_link_libraries(Luminous PUBLIC Qt5::Core Qt5::Network Qt5::Gui Qt5::Svg)

target_include_directories(Luminous PRIVATE ../ThirdParty/adl_sdk)
