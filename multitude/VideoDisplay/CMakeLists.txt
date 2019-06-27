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

  target_include_directories(VideoDisplay PRIVATE /opt/multitaction-ffmpeg40/include)
  target_link_directories(VideoDisplay PRIVATE /opt/multitaction-ffmpeg40/lib)
  target_link_libraries(VideoDisplay PRIVATE ${FFMPEG_LIBS})
endif()

set_target_properties(VideoDisplay PROPERTIES SOURCES "${SOURCES}")

target_compile_definitions(VideoDisplay PRIVATE -DVIDEODISPLAY_EXPORT)
target_compile_definitions(VideoDisplay PUBLIC -DENABLE_VIDEO_DISPLAY)

target_link_libraries(VideoDisplay PUBLIC Radiant)
