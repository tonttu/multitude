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


  target_link_libraries(${LIBRARY}
    PRIVATE
      ${MULTITACTION_DEPS_PATH}/manual/mwcapture/lib/LibMWCapture.lib
      MultiFFMPEG
      Strmiids.lib
      Propsys.lib
    )
endif()

target_compile_definitions(${LIBRARY} PRIVATE -DVIDEODISPLAY_EXPORT)
target_compile_definitions(${LIBRARY} PUBLIC -DENABLE_VIDEO_DISPLAY)

if(UNIX)
  # TODO: Should update our code that uses deprecated FFMPEG API
  # TODO: Should handle errors
  target_compile_options(${LIBRARY} PRIVATE
    -Wno-error=deprecated-declarations -Wno-error=unused-result
    )
endif()

target_link_libraries(${LIBRARY} PRIVATE Radiant Resonant)

cornerstone_install_lib(${LIBRARY})
