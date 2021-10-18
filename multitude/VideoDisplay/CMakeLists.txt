include(../../common.cmake)

add_library(VideoDisplay SHARED)

set(SOURCES
  AudioTransfer.cpp
  AVDecoder.cpp
  DummyDecoder.cpp
  FfmpegDecoder.cpp
  FfmpegVideoFormatSelector.cpp
)

if(UNIX)
  list(APPEND SOURCES
    FfmpegVideoFormatSelectorLinux.cpp
    V4L2Monitor.cpp
  )

  target_include_directories(VideoDisplay PRIVATE ${FFMPEG_ROOT}/include)

  find_library(AVDEVICE_LIB avdevice-multitaction PATHS ${FFMPEG_ROOT}/lib)
  find_library(AVCODEC_LIB avcodec-multitaction PATHS ${FFMPEG_ROOT}/lib)
  find_library(AVDEVICE_LIB avdevice-multitaction PATHS ${FFMPEG_ROOT}/lib)
  find_library(AVFILTER_LIB avfilter-multitaction PATHS ${FFMPEG_ROOT}/lib)
  find_library(AVFORMAT_LIB avformat-multitaction PATHS ${FFMPEG_ROOT}/lib)
  find_library(AVUTIL_LIB avutil-multitaction PATHS ${FFMPEG_ROOT}/lib)
  find_library(SWRESAMPLE_LIB swresample-multitaction PATHS ${FFMPEG_ROOT}/lib)
  find_library(SWSCALE_LIB swscale-multitaction PATHS ${FFMPEG_ROOT}/lib)
elseif(WIN32)
  list(APPEND SOURCES
    WindowsVideoMonitor.cpp
    WindowsVideoHelpers.cpp
    RGBEasy.cpp
    FfmpegVideoFormatSelectorWin.cpp
    MWCapture.cpp
  )

  #target_include_directories(VideoDisplay PRIVATE ${VCPKG_ROOT}/include)
  target_include_directories(VideoDisplay PRIVATE ${MULTITACTION_DEPS_PATH}/manual/mwcapture/include)
  target_include_directories(VideoDisplay PRIVATE ${CMAKE_CURRENT_LIST_DIR}/rgbeasy-sdk-v7.14.1/include)


  target_link_libraries(VideoDisplay PRIVATE ${MULTITACTION_DEPS_PATH}/manual/mwcapture/lib/LibMWCapture.lib)

  target_link_libraries(VideoDisplay PRIVATE Strmiids.lib Propsys.lib)

  find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h)
  target_include_directories(VideoDisplay PRIVATE ${FFMPEG_INCLUDE_DIR})

  find_library(AVDEVICE_LIB avdevice-multitaction)
  find_library(AVCODEC_LIB avcodec-multitaction)
  find_library(AVDEVICE_LIB avdevice-multitaction)
  find_library(AVFILTER_LIB avfilter-multitaction)
  find_library(AVFORMAT_LIB avformat-multitaction)
  find_library(AVUTIL_LIB avutil-multitaction)
  find_library(SWRESAMPLE_LIB swresample-multitaction)
  find_library(SWSCALE_LIB swscale-multitaction)
endif()

set_target_properties(VideoDisplay PROPERTIES SOURCES "${SOURCES}")

target_compile_definitions(VideoDisplay PRIVATE -DVIDEODISPLAY_EXPORT)
target_compile_definitions(VideoDisplay PUBLIC -DENABLE_VIDEO_DISPLAY)

target_link_libraries(VideoDisplay PUBLIC Radiant Resonant)

# FFMPEG
target_link_libraries(VideoDisplay PRIVATE ${AVCODEC_LIB} ${AVDEVICE_LIB} ${AVFILTER_LIB} ${AVFORMAT_LIB} ${AVUTIL_LIB} ${SWSCALE_LIB})
