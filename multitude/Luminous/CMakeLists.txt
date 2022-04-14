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
  BezierSplineTessellator.cpp
  BezierSplineRenderer.cpp
  BezierSpline.cpp
  ImageCodecQT.cpp
  ImageCodecSVG.cpp
  ScreenDetectorAMD.cpp
  ScreenDetectorNV.cpp
  UploadBuffer.cpp
  # For Linux
  $<$<PLATFORM_ID:Linux>:XRandR.cpp>
  $<$<PLATFORM_ID:Linux>:Xinerama.cpp>
  # For Windows
  $<$<PLATFORM_ID:Windows>:GPUAffinity.cpp>
  $<$<PLATFORM_ID:Windows>:DisplayConfigWin.cpp>
  $<$<PLATFORM_ID:Windows>:DxInterop.cpp>
  $<$<PLATFORM_ID:Windows>:DxSharedTexture.cpp>
)

target_compile_definitions(${LIBRARY} PRIVATE -DLUMINOUS_EXPORT)

target_link_libraries(${LIBRARY}
  PRIVATE
    Squish
    MultiLz4
    NVML
    ADL_SDK
    Qt5::Widgets
    Qt5::Svg
    $<$<PLATFORM_ID:Linux>:Qt5::X11Extras>
    "$<$<PLATFORM_ID:Linux>:XNVCtrl;Xrandr;Xext;Xinerama>"
    "$<$<PLATFORM_ID:Windows>:Opengl32;Dwmapi;DXGI;D3D11;WindowsApp>"
    $<$<PLATFORM_ID:Windows>:MultiNVApi>
  PUBLIC
    Valuable
    Nimble
    Punctual
    Patterns
    Qt5::Core
)

cornerstone_install_lib(${LIBRARY})
