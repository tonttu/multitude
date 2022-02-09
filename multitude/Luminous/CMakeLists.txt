set(LIBRARY Luminous)

cornerstone_add_library(${LIBRARY} SHARED)

target_sources(${LIBRARY} PRIVATE
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

if(ENABLE_PDF)
  target_sources(${LIBRARY} PRIVATE PDFManager.cpp)

  target_link_libraries(${LIBRARY}
    PRIVATE OurExpected
    PUBLIC Punctual
  )

  cornerstone_add_pdfium_libs(${LIBRARY} PRIVATE)
endif()

if(UNIX)

  target_sources(${LIBRARY} PRIVATE
    XRandR.cpp
    Xinerama.cpp
  )

  target_link_libraries(${LIBRARY} PRIVATE XNVCtrl Xrandr Xext Xinerama)

  find_library(LZ4_LIBRARY lz4)
  target_link_libraries(${LIBRARY} PRIVATE lz4)

  cornerstone_add_qt_libs(${LIBRARY} PRIVATE "X11Extras")
elseif(WIN32)

  target_sources(${LIBRARY} PRIVATE
    GPUAffinity.cpp
    DisplayConfigWin.cpp
    DxInterop.cpp
    DxSharedTexture.cpp
  )

  target_include_directories(${LIBRARY} PRIVATE ${MULTITACTION_DEPS_PATH}/manual/nvapi/include)
  target_link_libraries(${LIBRARY} PRIVATE ${MULTITACTION_DEPS_PATH}/manual/nvapi/lib/nvapi64.lib)

  target_link_libraries(${LIBRARY} PRIVATE Opengl32.lib Dwmapi.lib DXGI.lib D3D11.lib WindowsApp.lib)

  find_package(lz4 CONFIG REQUIRED)
  target_link_libraries(${LIBRARY} PRIVATE lz4::lz4)
endif()

target_compile_definitions(${LIBRARY} PRIVATE -DLUMINOUS_EXPORT)

target_link_libraries(${LIBRARY} PRIVATE Squish)
target_link_libraries(${LIBRARY} PUBLIC Valuable Nimble)

cornerstone_add_qt_libs(${LIBRARY} PUBLIC "Network;Gui;Svg")

target_include_directories(${LIBRARY} PRIVATE ../ThirdParty/adl_sdk ../ThirdParty)

cornerstone_install_lib(${LIBRARY})
