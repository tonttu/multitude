include(../multitude.pri)

HEADERS += Export.hpp \
    Frame4.hpp
HEADERS += RollingAverage.hpp
HEADERS += Histogram.hpp 
HEADERS += Interpolation.hpp
HEADERS += KeyStone.hpp
HEADERS += LensCorrection.hpp
HEADERS += LineSegment2.hpp
HEADERS += Math.hpp
HEADERS += Matrix2.hpp
HEADERS += Matrix3.hpp
HEADERS += Matrix4.hpp
HEADERS += Nimble.hpp
HEADERS += Path.hpp
HEADERS += Plane.hpp
HEADERS += Quaternion.hpp
HEADERS += Ramp.hpp
HEADERS += Random.hpp
HEADERS += Range.hpp
HEADERS += Rectangle.hpp
HEADERS += Rect.hpp
HEADERS += Splines.hpp
HEADERS += Vector2.hpp
HEADERS += Vector3.hpp
HEADERS += Vector4.hpp

# SOURCES += Histogram.cpp
SOURCES += RollingAverage.cpp
SOURCES += KeyStone.cpp
SOURCES += LensCorrection.cpp
SOURCES += Path.cpp
SOURCES += Plane.cpp
SOURCES += Random.cpp
SOURCES += Rectangle.cpp
SOURCES += Splines.cpp

DEFINES += NIMBLE_EXPORT

include(../library.pri)
