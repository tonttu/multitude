# Install Windows 3rd party dlls to bin
win64_runtime_dlls.path = /bin
win64_runtime_dlls.files = $$PWD/bin64/*

# Install libraries under 'src'
win64_sdk_libs1.path = /src/multitude/Win64x/lib64
win64_sdk_libs1.files = $$PWD/lib64/*

# Install libraries under 'lib'
win64_sdk_libs2.path = /lib
win64_sdk_libs2.files = $$PWD/lib64/*

# Install dlls under 'src'
win64_sdk_dlls.path = /src/multitude/Win64x/bin64
win64_sdk_dlls.files = $$PWD/bin64/*

# Install headers under 'src'
win64_sdk_headers1.path = /src/multitude/Win64x/include
win64_sdk_headers1.files = $$PWD/include/*

# Install headers under 'include'
win64_sdk_headers2.path = /include
win64_sdk_headers2.files = $$PWD/include/*

# Install project files under 'src'
win64_sdk_project.path = /src/multitude/Win64x
win64_sdk_project.files = $$PWD/Win64x.pri

exists("C:/Cornerstone-deps/libav/bin") {
  win64_libav_dlls1.path = /bin
  win64_libav_dlls1.files = C:/Cornerstone-deps/libav/bin/*dll

  win64_libav_dlls2.path = /src/multitude/Win64x/bin64
  win64_libav_dlls2.files = C:/Cornerstone-deps/libav/bin/*dll

  win64_libav_libs1.path = /lib
  win64_libav_libs1.files = C:/Cornerstone-deps/libav/bin/*lib

  win64_libav_libs2.path = /src/multitude/Win64x/lib64
  win64_libav_libs2.files = C:/Cornerstone-deps/libav/bin/*lib

  win64_libav_headers1.path = /include
  win64_libav_headers1.files = C:/Cornerstone-deps/libav/include/*

  win64_libav_headers2.path = /src/multitude/Win64x/include
  win64_libav_headers2.files = C:/Cornerstone-deps/libav/include/*

  INSTALLS += win64_libav_dlls1
  INSTALLS += win64_libav_dlls2
  INSTALLS += win64_libav_libs1
  INSTALLS += win64_libav_libs2
  INSTALLS += win64_libav_headers1
  INSTALLS += win64_libav_headers2
}

exists("C:/Cornerstone-deps/ghostscript/gswin64c.exe") {

  win64_ghostscript.path = /bin
  win64_ghostscript.files = C:/Cornerstone-deps/ghostscript/*
  
  INSTALLS += win64_ghostscript
}

exists("C:/Cornerstone-deps/node/bin") {
  win64_node_dlls1.path = /bin
  win64_node_dlls1.files = C:/Cornerstone-deps/node/bin/node.exe
  win64_node_dlls1.files += C:/Cornerstone-deps/node/bin/*.cmd
  win64_node_dlls1.files += C:/Cornerstone-deps/node/bin/*.dll
  win64_node_dlls1.files += C:/Cornerstone-deps/node/bin/node_modules

  win64_node_dlls2.path = /src/multitude/Win64x/bin64
  win64_node_dlls2.files = C:/Cornerstone-deps/node/bin/node.exe
  win64_node_dlls2.files += C:/Cornerstone-deps/node/bin/*.cmd
  win64_node_dlls2.files += C:/Cornerstone-deps/node/bin/*.dll
  win64_node_dlls2.files += C:/Cornerstone-deps/node/bin/node_modules

  win64_node_libs1.path = /lib
  win64_node_libs1.files = C:/Cornerstone-deps/node/lib/*lib

  win64_node_libs2.path = /src/multitude/Win64x/lib64
  win64_node_libs2.files = C:/Cornerstone-deps/node/lib/*lib

  win64_node_headers1.path = /include
  win64_node_headers1.files = C:/Cornerstone-deps/node/include/*

  win64_node_headers2.path = /src/multitude/Win64x/include
  win64_node_headers2.files = C:/Cornerstone-deps/node/include/*

  INSTALLS += win64_node_dlls1
  INSTALLS += win64_node_dlls2
  INSTALLS += win64_node_libs1
  INSTALLS += win64_node_libs2
  INSTALLS += win64_node_headers1
  INSTALLS += win64_node_headers2
}

INSTALLS += win64_runtime_dlls
INSTALLS += win64_sdk_libs1
INSTALLS += win64_sdk_libs2
INSTALLS += win64_sdk_dlls
INSTALLS += win64_sdk_headers1
INSTALLS += win64_sdk_headers2
INSTALLS += win64_sdk_project

# Install Qt
qt_bin_files.path = /
qt_bin_files.files = $$[QT_INSTALL_BINS]

qt_conf_files.path = /bin
qt_conf_files.files = qt.conf

qt_lib_files.path = /qt/lib
qt_lib_files.files = $$[QT_INSTALL_LIBS]\\*.lib

qt_files.path = /qt
qt_files.files += $$[QT_INSTALL_PLUGINS]
qt_files.files += $$[QT_INSTALL_IMPORTS]
qt_files.files += $$[QT_INSTALL_TRANSLATIONS]
qt_files.files += $$[QMAKE_MKSPECS]
qt_files.files += $$[QT_INSTALL_HEADERS]

INSTALLS += qt_bin_files qt_lib_files qt_include_files qt_files qt_conf_files

message(Including 64-bit Windows Libraries)
