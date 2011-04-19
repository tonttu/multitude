include(../multitude.pri)

FBX_SDK=$$(FBX_SDK)

INCLUDEPATH += $$FBX_SDK/include
LIBPATH += $$FBX_SDK/lib/gcc4
LIBS += -lfbxsdk_20113_1_x64

HEADERS += Scene.hpp \
    DrawUtils.hpp \
    Mesh.hpp \
    Material.hpp \
    TextureManager.hpp \
    MeshManager.hpp \
    Renderable.hpp

SOURCES += Scene.cpp \
    DrawUtils.cpp \
    Mesh.cpp \
    TextureManager.cpp \
    MeshManager.cpp \
    Renderable.cpp

include(../library.pri)
