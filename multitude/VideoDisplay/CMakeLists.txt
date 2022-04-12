set(LIBRARY VideoDisplay)

cornerstone_add_library(${LIBRARY} SHARED)

target_sources(${LIBRARY} PRIVATE
  AudioTransfer.cpp
  AVDecoder.cpp
  DummyDecoder.cpp
  FfmpegDecoder.cpp
  FfmpegVideoFormatSelector.cpp
  # For Linux
  $<$<PLATFORM_ID:Linux>:FfmpegVideoFormatSelectorLinux.cpp>
  $<$<PLATFORM_ID:Linux>:V4L2Monitor.cpp>
  # For Windows
  $<$<PLATFORM_ID:Windows>:WindowsVideoMonitor.cpp>
  $<$<PLATFORM_ID:Windows>:WindowsVideoHelpers.cpp>
  $<$<PLATFORM_ID:Windows>:RGBEasy.cpp>
  $<$<PLATFORM_ID:Windows>:FfmpegVideoFormatSelectorWin.cpp>
  $<$<PLATFORM_ID:Windows>:MWCapture.cpp>
)

target_compile_definitions(${LIBRARY}
  PRIVATE VIDEODISPLAY_EXPORT
  PUBLIC ENABLE_VIDEO_DISPLAY
)

# TODO: Should update our code that uses deprecated FFMPEG API
# TODO: Should handle errors
target_compile_options(${LIBRARY} PRIVATE
  $<$<PLATFORM_ID:Linux>:-Wno-error=deprecated-declarations>
  $<$<PLATFORM_ID:Linux>:-Wno-error=unused-result>
)

target_link_libraries(${LIBRARY}
  PRIVATE
    Radiant
    Resonant
    MultiFFMPEG
    "$<$<PLATFORM_ID:Windows>:RGBEasy;MultiMWCapture;Strmiids;Propsys>"
)

cornerstone_install_lib(${LIBRARY})
