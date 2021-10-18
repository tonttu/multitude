include(../../../common.cmake)

add_executable(ListPortAudioDevices Main.cpp)

if(UNIX)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

  target_link_libraries(ListPortAudioDevices PRIVATE ${PORTAUDIO_LIBRARIES})
elseif(WIN32)
  target_include_directories(ListPortAudioDevices PRIVATE ${VCPKG_ROOT}/include)
  target_link_libraries(ListPortAudioDevices PRIVATE ${VCPKG_ROOT}/lib/portaudio.lib)
endif()

find_package(Qt5 COMPONENTS Core PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ListPortAudioDevices PRIVATE Qt5::Core)
