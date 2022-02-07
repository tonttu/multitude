set(LIBRARY VideoDisplay)

cornerstone_add_library(${LIBRARY} SHARED)

target_sources(${LIBRARY} PRIVATE
  AudioTransfer.cpp
  AVDecoder.cpp
  DummyDecoder.cpp
  FfmpegDecoder.cpp
  FfmpegVideoFormatSelector.cpp
)

if(UNIX)
  target_sources(${LIBRARY} PRIVATE
    FfmpegVideoFormatSelectorLinux.cpp
    V4L2Monitor.cpp
  )

  target_link_libraries(${LIBRARY} PRIVATE PkgConfig::MultiFFMPEG)
elseif(WIN32)
  target_sources(${LIBRARY} PRIVATE
    WindowsVideoMonitor.cpp
    WindowsVideoHelpers.cpp
    RGBEasy.cpp
    FfmpegVideoFormatSelectorWin.cpp
    MWCapture.cpp
  )

  #target_include_directories(${LIBRARY} PRIVATE ${VCPKG_ROOT}/include)
  target_include_directories(${LIBRARY} PRIVATE ${MULTITACTION_DEPS_PATH}/manual/mwcapture/include)
  target_include_directories(${LIBRARY} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/rgbeasy-sdk-v7.14.1/include)


  target_link_libraries(${LIBRARY} PRIVATE ${MULTITACTION_DEPS_PATH}/manual/mwcapture/lib/LibMWCapture.lib)

  target_link_libraries(${LIBRARY} PRIVATE Strmiids.lib Propsys.lib)

  find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h)
  target_include_directories(${LIBRARY} PRIVATE ${FFMPEG_INCLUDE_DIR})

  # FFMPEG
  find_library(AVDEVICE_LIB avdeviceMT REQUIRED)
  find_library(AVCODEC_LIB avcodecMT REQUIRED)
  find_library(AVFILTER_LIB avfilterMT REQUIRED)
  find_library(AVFORMAT_LIB avformatMT REQUIRED)
  find_library(AVUTIL_LIB avutilMT REQUIRED)
  find_library(SWRESAMPLE_LIB swresampleMT REQUIRED)
  find_library(SWSCALE_LIB swscaleMT REQUIRED)

  target_link_libraries(${LIBRARY} PRIVATE
    ${AVCODEC_LIB} ${AVDEVICE_LIB} ${AVFILTER_LIB} ${AVFORMAT_LIB}
    ${AVUTIL_LIB} ${SWSCALE_LIB}
  )
endif()

target_compile_definitions(${LIBRARY} PRIVATE -DVIDEODISPLAY_EXPORT)
target_compile_definitions(${LIBRARY} PUBLIC -DENABLE_VIDEO_DISPLAY)

target_link_libraries(${LIBRARY} PRIVATE Radiant Resonant)

cornerstone_install_lib(${LIBRARY})
