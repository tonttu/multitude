include(../../common.cmake)

set(LIBRARY Luminous)

cornerstone_add_library(${LIBRARY} SHARED)

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

if(ENABLE_PDF)
  list(APPEND SOURCES PDFManager.cpp)

  target_include_directories(${LIBRARY} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../../multitude/ThirdParty
    ${CMAKE_CURRENT_LIST_DIR}/../../multitude/ThirdParty/expected/include
  )
endif()

if(UNIX)

  list(APPEND SOURCES
    XRandR.cpp
    Xinerama.cpp
  )

  if(ENABLE_PDF)
    target_include_directories(${LIBRARY} PRIVATE /opt/multitaction-pdfium2/include)
    target_link_libraries(${LIBRARY}
      PUBLIC Punctual
      PRIVATE /opt/multitaction-pdfium2/lib/libmultitaction-pdfium2.a
    )
  endif()

  find_package(Qt5 COMPONENTS X11Extras PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
  target_link_libraries(${LIBRARY} PRIVATE Qt5::X11Extras)
  target_link_libraries(${LIBRARY} PRIVATE XNVCtrl Xrandr Xext Xinerama)

  find_library(LZ4_LIBRARY lz4)
  target_link_libraries(${LIBRARY} PRIVATE lz4)
elseif(WIN32)

  list(APPEND SOURCES
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

set_target_properties(${LIBRARY} PROPERTIES SOURCES "${SOURCES}")

target_compile_definitions(${LIBRARY} PRIVATE -DLUMINOUS_EXPORT)

find_package(Qt5 COMPONENTS Core PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Network PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Gui PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Svg PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

target_link_libraries(${LIBRARY} PRIVATE Squish)
target_link_libraries(${LIBRARY} PUBLIC Valuable Nimble)
target_link_libraries(${LIBRARY} PUBLIC Qt5::Core Qt5::Network Qt5::Gui Qt5::Svg)

target_include_directories(${LIBRARY} PRIVATE ../ThirdParty/adl_sdk ../ThirdParty)

cornerstone_install_lib(${LIBRARY})
