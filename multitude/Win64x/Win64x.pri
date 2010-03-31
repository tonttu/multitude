# Install Windows 3rd party dlls to bin
win64_runtime_dlls.path = /bin
win64_runtime_dlls.files = $$PWD/bin64/*

win64_sdk_libs.path = /src/MultiTouch/multitude/Win64x/lib64
win64_sdk_libs.files = $$PWD/lib64/*

win64_sdk_dlls.path = /src/MultiTouch/multitude/Win64x/bin64
win64_sdk_dlls.files = $$PWD/bin64/*

win64_sdk_headers.path = /src/MultiTouch/multitude/Win64x/include
win64_sdk_headers.files = $$PWD/include/*

win64_sdk_project.path = /src/MultiTouch/multitude/Win64x
win64_sdk_project.files = $$PWD/Win64x.pri

INSTALLS += win64_runtime_dlls
INSTALLS += win64_sdk_libs
INSTALLS += win64_sdk_dlls
INSTALLS += win64_sdk_headers
INSTALLS += win64_sdk_project

message(Including 64-bit Windows Libraries)