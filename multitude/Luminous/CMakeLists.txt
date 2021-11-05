include(../../common.cmake)

add_library(Luminous SHARED)

set(SOURCES
  ImageCodecDDS.cpp
  GPUAssociation.cpp
  MaskGuard.cpp
  MipmapRenderer.cpp
  ScreenDetectorQt.cpp
  SwapGroups.cpp
  MemoryManager.cpp
  Nvml.cpp
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
  UploadBuffer.cpp
)

if(UNIX)

  list(APPEND SOURCES
    XRandR.cpp
    Xinerama.cpp
  )

  if(ENABLE_PDF)
    target_include_directories(Luminous PRIVATE /opt/multitaction-pdfium2/include)
    target_link_libraries(Luminous PRIVATE Punctual)
  endif()

  find_package(Qt5 COMPONENTS X11Extras PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
  target_link_libraries(Luminous PRIVATE Qt5::X11Extras)
  target_link_libraries(Luminous PRIVATE XNVCtrl Xrandr Xext Xinerama)
elseif(WIN32)

  list(APPEND SOURCES
    GPUAffinity.cpp
    DisplayConfigWin.cpp
    DxInterop.cpp
  )

  target_include_directories(Luminous PRIVATE ${MULTITACTION_DEPS_PATH}/manual/nvapi/include)
  target_link_libraries(Luminous PRIVATE ${MULTITACTION_DEPS_PATH}/manual/nvapi/lib/nvapi64.lib)

  target_link_libraries(Luminous PRIVATE Opengl32.lib Dwmapi.lib)

endif()

set_target_properties(Luminous PROPERTIES SOURCES "${SOURCES}")

target_compile_definitions(Luminous PRIVATE -DLUMINOUS_EXPORT)

find_package(Qt5 COMPONENTS Core PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Network PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Gui PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Svg PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

target_link_libraries(Luminous PRIVATE Squish)
target_link_libraries(Luminous PUBLIC Valuable Nimble)
target_link_libraries(Luminous PUBLIC Qt5::Core Qt5::Network Qt5::Gui Qt5::Svg)

target_include_directories(Luminous PRIVATE ../ThirdParty/adl_sdk ../ThirdParty)

find_library(LZ4_LIBRARY lz4)
target_link_libraries(Luminous PRIVATE lz4)
