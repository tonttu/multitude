project ("Nimble")
  kind "SharedLib"
  defines {"NIMBLE_EXPORT"}
  files  { "**.hpp", "**.cpp" }
  includedirs { "." }