project ("Luminous")
  kind "SharedLib"
  defines {"LUMINOUS_EXPORT"}
  files  { "**.hpp", "**.cpp" }
  includedirs { "." }
  includedirs { "../Externals/adl_sdk" }

  links{"Radiant","Valuable","Nimble","Patterns", "Squish"}
  links{QtSvg}
  links{OpenGL}

  -- Deprecated, should be removed
  excludes { "TiledMipMapImage.cpp" }
  
  configuration{"windows"}
    excludes{"XRandR.cpp"}
  configuration{}

  configuration{"linux"}
    links{"Xrandr", "XNVCtrl"}
  configuration{}

  -- OpenGL and NVApi
  configuration{"windows", "x64"}
    links{"nvapi64"}
  configuration{"windows", "x32"}
    links{"nvapi"}
  configuration{}
