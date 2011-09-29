include(../multitude.pri)

FBX_SDK=$$(FBX_SDK)

INCLUDEPATH += $$FBX_SDK/include
QMAKE_LIBDIR += $$FBX_SDK/lib/gcc4
LIBS += -lfbxsdk_20113_1_x64

DEFINES += VIVID_EXPORT

HEADERS += Scene.hpp \
    DrawUtils.hpp \
    Mesh.hpp \
    Material.hpp \
    TextureManager.hpp \
    MeshManager.hpp \
    Renderable.hpp \
    Camera.hpp \
    Transform.hpp \
    Light.hpp \
    Triangle.hpp

SOURCES += Scene.cpp \
    DrawUtils.cpp \
    Mesh.cpp \
    TextureManager.cpp \
    MeshManager.cpp \
    Renderable.cpp \
    Camera.cpp \
    Transform.cpp \
    Light.cpp

include(../library.pri)
