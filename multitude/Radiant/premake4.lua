project ("Radiant")
  kind "SharedLib"
  defines {"RADIANT_EXPORT"}
  files  { "**.hpp", "**.cpp" }
  -- Deprecated files, should be deleted
  excludes { "TCPServerSocket.cpp", "TCPServerSocketQt.cpp", "TCPSocketQt.cpp", "UDPSocketQt.cpp" }
  
  includedirs { "." }

  links {"Nimble", "Patterns"}
  links {QtCore}
 
  configuration { "linux" }
    links{"dl", "Xtst"}
    linkoptions{"`pkg-config --libs libdc1394-2`"}
    buildoptions{"`pkg-config --cflags libdc1394-2`"}
    defines{"CAMERA_DRIVER_1394"}
  configuration { "windows", "x32"}
    defines{"CAMERA_DRIVER_CMU"}
  configuration { "windows", "x64"}
    defines{"CAMERA_DRIVER_PGR"}
  configuration { "windows"}
    links {"FlyCapture2"}
    links {"Ws2_32", "Psapi", "Shlwapi"}
    includedirs { "C:/Program Files/Point Grey Research/FlyCapture2/include" }
  configuration {}
