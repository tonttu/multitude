project ("Luminous")
  kind "SharedLib"
  defines {"LUMINOUS_EXPORT"}
  files  { "**.hpp", "**.cpp" }
  includedirs { "." }
  excludes { "TiledMipMapImage.cpp" }
  includedirs { "../Externals/adl_sdk" }
  
  links{"Radiant","Valuable","Nimble","Patterns"}
  links{"OpenGL32", "GLU32","glew64","nvapi64", "Squish"}
  
  configuration{"windows"}
    excludes{"XRandR.cpp"}
	
