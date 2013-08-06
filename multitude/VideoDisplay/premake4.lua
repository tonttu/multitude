project ("VideoDisplay")
  kind "SharedLib"
  defines {"VIDEODISPLAY_EXPORT"}
  files  { "**.hpp", "**.cpp" }
  includedirs { "." }
  links{"Radiant", "Resonant", "Valuable", "Luminous", "Nimble", "Screenplay", "Patterns", "Poetic", OpenGL}
