project ("Patterns")
  kind "SharedLib"
  defines {"PATTERNS_EXPORT"}
  files  { "**.hpp", "**.cpp" }
  includedirs { "." }