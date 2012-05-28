project("MoviePlayer")
  kind "ConsoleApp"
  files{"**.hpp", "**.cpp"}
  links{"VideoDisplay", "Poetic", "Resonant", "Screenplay", "Luminous"}
  links{QtCore, QtGui, QtOpenGL, QtXML}
