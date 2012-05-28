project("FireView")
  kind "ConsoleApp"
  files{"**.hpp", "**.cpp"}
  links{"Radiant", "Valuable", "Luminous", "Nimble", "Patterns"}
  links{QtCore, QtGui, QtOpenGL, QtXML}
  links{OpenGL}
