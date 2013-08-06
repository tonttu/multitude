project ("Resonant")
  kind "SharedLib"
  defines {"RESONANT_EXPORT"}
  files  { "**.hpp", "**.cpp" }
  includedirs { "." }
  
  links {"Radiant", "Valuable", "Nimble", "Patterns"}

  -- PortAudio
  configuration { "windows" }
    links {"libsndfile-1", "portaudio" }
  configuration { "windows", "x64" }
    --libdir { "QMAKE_LIBDIR += $$DDK_PATH\\lib\\win7\\amd64
    includedirs { "../Win64x/include/portaudio" }
  configuration { "windows", "x32" }
    includedirs { "../Win32x/include/portaudio" }
  configuration {}
