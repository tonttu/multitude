project ("Squish")
  kind "StaticLib"
  files  { "*.hpp", "*.cpp" }
  includedirs { "." }

  configuration { "linux" }
    buildoptions{"-fPIC"}
  configuration { }
