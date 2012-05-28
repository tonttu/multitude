project("ListPortAudioDevices")
  kind "ConsoleApp"
  files{"**.hpp", "**.cpp"}

  configuration {"linux"}
    linkoptions{"`pkg-config --libs portaudio-2.0`"}
    buildoptions{"`pkg-config --cflags portaudio-2.0`"}
  configuration {"windows", "x64"}
    includedirs {"../../Win64x/include/portaudio"}
    includedirs {"../../Win64x/include/libsndfile"}
    links{"libsndfile-1", "portaudio"}
  configuration {"windows", "x32"}
    includedirs {"../../Win32x/include/portaudio"}
    includedirs {"../../Win32x/include/libsndfile"}
    links{"libsndfile-1", "portaudio_x86"}
  configuration {}
