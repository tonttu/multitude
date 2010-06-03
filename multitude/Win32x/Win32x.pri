# Install Windows 3rd party dlls to bin
win32_runtime_dlls.path = /bin
win32_runtime_dlls.files = $$PWD/bin32/*

# Install libraries under 'src'
win32_sdk_libs1.path = /src/MultiTouch/multitude/Win32x/lib32
win32_sdk_libs1.files = $$PWD/lib32/*

# Install libraries under 'lib'
win32_sdk_libs2.path = /lib
win32_sdk_libs2.files = $$PWD/lib32/*

# Install dlls under 'src'
win32_sdk_dlls.path = /src/MultiTouch/multitude/Win32x/bin32
win32_sdk_dlls.files = $$PWD/bin32/*

# Install headers under 'src'
win32_sdk_headers1.path = /src/MultiTouch/multitude/Win32x/include
win32_sdk_headers1.files = $$PWD/include/*

# Install headers under 'include'
win32_sdk_headers2.path = /include
win32_sdk_headers2.files = $$PWD/include/*

# Install project files under 'src'
win32_sdk_project.path = /src/MultiTouch/multitude/Win32x
win32_sdk_project.files = $$PWD/win32x.pri

INSTALLS += win32_runtime_dlls
INSTALLS += win32_sdk_libs1
INSTALLS += win32_sdk_libs2
INSTALLS += win32_sdk_dlls
INSTALLS += win32_sdk_headers1
INSTALLS += win32_sdk_headers2
INSTALLS += win32_sdk_project

message(Including 32-bit Windows Libraries)