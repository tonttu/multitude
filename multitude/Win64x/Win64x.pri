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

INSTALLS += win64_runtime_dlls
INSTALLS += win64_sdk_libs1
INSTALLS += win64_sdk_libs2
INSTALLS += win64_sdk_dlls
INSTALLS += win64_sdk_headers1
INSTALLS += win64_sdk_headers2
INSTALLS += win64_sdk_project

message(Including 64-bit Windows Libraries)
