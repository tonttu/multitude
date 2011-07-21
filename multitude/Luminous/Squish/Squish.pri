SOURCES += Squish/alpha.cpp
SOURCES += Squish/clusterfit.cpp
SOURCES += Squish/colourblock.cpp
SOURCES += Squish/colourfit.cpp
SOURCES += Squish/colourset.cpp
SOURCES += Squish/maths.cpp
SOURCES += Squish/rangefit.cpp
SOURCES += Squish/singlecolourfit.cpp
SOURCES += Squish/squish.cpp

HEADERS += Squish/alpha.h
HEADERS += Squish/clusterfit.h
HEADERS += Squish/colourblock.h
HEADERS += Squish/colourfit.h
HEADERS += Squish/colourset.h
HEADERS += Squish/maths.h
HEADERS += Squish/rangefit.h
HEADERS += Squish/simd_float.h
HEADERS += Squish/simd.h
HEADERS += Squish/simd_sse.h
HEADERS += Squish/simd_ve.h
HEADERS += Squish/singlecolourfit.h
HEADERS += Squish/squish.h

DEFINES += SQUISH_USE_SSE=2
INCLUDEPATH += $$PWD
