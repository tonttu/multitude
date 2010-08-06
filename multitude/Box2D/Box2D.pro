include(../multitude.pri)

TEMPLATE = lib

CONFIG -= qt

CONFIG -= debug
CONFIG += release

SOURCES += Box2D/Collision/Shapes/*.cpp
SOURCES += Box2D/Collision/*.cpp
SOURCES += Box2D/Common/*.cpp
SOURCES += Box2D/Dynamics/*.cpp
SOURCES += Box2D/Dynamics/Contacts/*.cpp
SOURCES += Box2D/Dynamics/Joints/*.cpp

HEADERS += Box2D/*.h
HEADERS += Box2D/Common/*.h
HEADERS += Box2D/Dynamics/*.h
HEADERS += Box2D/Dynamics/Contacts/*.h
HEADERS += Box2D/Dynamics/Joints/*.h

unix:QMAKE_CXXFLAGS_RELEASE += -ffast-math

include(../library.pri)
