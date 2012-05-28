project ("Screenplay")
  kind "SharedLib"
  defines {"SCREENPLAY_EXPORT"}
  files  { "**.hpp", "**.cpp" }
  includedirs { "." }

  links {"Radiant", "Nimble"}

  -- Qt
  links {QtCore}
  -- FFMpeg 
  links {FFmpeg}
