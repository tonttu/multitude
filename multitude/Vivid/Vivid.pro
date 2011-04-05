include(../multitude.pri)

FBX_SDK=/home/aki/fbx

INCLUDEPATH += $$FBX_SDK/include
LIBPATH += $$FBX_SDK/lib/gcc4
LIBS += -lfbxsdk_20113_1_x64

HEADERS += Scene.hpp \
    DrawUtils.hpp

SOURCES += Scene.cpp \
    DrawUtils.cpp

include(../library.pri)
