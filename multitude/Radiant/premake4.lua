project ("Radiant")
  kind "SharedLib"
  defines {"RADIANT_EXPORT"}
  files  { "**.hpp", "**.cpp" }
  excludes { "TCPServerSocket.cpp", "TCPServerSocketQt.cpp", "TCPSocketQt.cpp", "UDPSocketQt.cpp" }
  
  includedirs { "." }
  links {"Patterns", "Ws2_32", "Psapi", "Shlwapi", "QtCored4", "OpenGL32", "GLU32", "FlyCapture2" }

  includedirs { "C:/Program Files/Point Grey Research/FlyCapture2/include" }
  
  links{"Nimble"}