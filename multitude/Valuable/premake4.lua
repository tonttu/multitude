project ("Valuable")
  kind "SharedLib"
  defines {"VALUABLE_EXPORT"}
  files  { "**.hpp", "**.cpp" }
  includedirs { "." }
  
  links {"Radiant", "Patterns", "Nimble"}
  links {QtXml}
