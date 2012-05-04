project ("Screenplay")
  kind "SharedLib"
  defines {"SCREENPLAY_EXPORT"}
  files  { "**.hpp", "**.cpp" }
  includedirs { "." }
  links { "avutil" ,"avformat", "avcodec", "Radiant", "Nimble", "QtCored4" }
  
  configuration { "windows", "x64" }
    includedirs { "../Win64x/include/ffmpeg" }
  configuration { "windows", "x32" }
    includedirs { "../Win32x/include/ffmpeg" }