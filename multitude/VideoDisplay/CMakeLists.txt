include(../../common.cmake)

project(VideoDisplay VERSION ${CORNERSTONE_VERSION} DESCRIPTION "a C++ library for playing movie files using OpenGL hardware.")

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
endif()

set_target_properties(VideoDisplay PROPERTIES SOURCES "${SOURCES}")

add_definitions(-DVIDEODISPLAY_EXPORT)

find_package(Qt5 COMPONENTS Core PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)
find_package(Qt5 COMPONENTS Gui PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

target_link_libraries(VideoDisplay Qt5::Core)

target_include_directories(VideoDisplay PUBLIC ${Qt5Gui_INCLUDE_DIRS})
