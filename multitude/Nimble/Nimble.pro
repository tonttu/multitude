include(../multitude.pri)

HEADERS += Export.hpp
HEADERS += Histogram.hpp 
HEADERS += KeyStone.hpp 
HEADERS += LensCorrection.hpp 
HEADERS += LineSegment2.hpp 
HEADERS += Math.hpp 
HEADERS += Matrix2.hpp 
HEADERS += Matrix2Impl.hpp
HEADERS += Matrix3.hpp 
HEADERS += Matrix3Impl.hpp 
HEADERS += Matrix4.hpp 
HEADERS += Matrix4Impl.hpp
HEADERS += Nimble.hpp
HEADERS += Plane.hpp
HEADERS += Ramp.hpp 
HEADERS += Random.hpp 
HEADERS += Rectangle.hpp
HEADERS += Rect.hpp 
HEADERS += Splines.hpp
HEADERS += SplinesImpl.hpp
HEADERS += Vector2.hpp 
HEADERS += Vector3.hpp 
HEADERS += Vector4.hpp 

SOURCES += Histogram.cpp
SOURCES += KeyStone.cpp
SOURCES += LensCorrection.cpp
SOURCES += LineSegment2.cpp 
SOURCES += Matrix.cpp 
SOURCES += Plane.cpp
SOURCES += Random.cpp
SOURCES += Rectangle.cpp
SOURCES += Rect.cpp
SOURCES += Splines.cpp
SOURCES += Vector2.cpp

win32:DEFINES += NIMBLE_EXPORT

include(../library.pri)
