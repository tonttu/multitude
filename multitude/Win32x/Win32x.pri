# Install Windows 3rd party dlls to bin
win32_runtime_dlls.path = /bin
win32_runtime_dlls.files = $$PWD/bin32/*

win32_sdk_libs.path = /src/MultiTouch/multitude/Win32x/lib32
win32_sdk_libs.files = $$PWD/lib32/*

win32_sdk_dlls.path = /src/MultiTouch/multitude/Win32x/bin32
win32_sdk_dlls.files = $$PWD/bin32/*

win32_sdk_headers.path = /src/MultiTouch/multitude/Win32x/include
win32_sdk_headers.files = $$PWD/include/*

win32_sdk_project.path = /src/MultiTouch/multitude/Win32x
win32_sdk_project.files = $$PWD/win32x.pri

INSTALLS += win32_runtime_dlls
INSTALLS += win32_sdk_libs
INSTALLS += win32_sdk_dlls
INSTALLS += win32_sdk_headers
INSTALLS += win32_sdk_project

message(Including 32-bit Windows Libraries)