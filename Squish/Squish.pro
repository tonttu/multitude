include(../../cornerstone.pri)

TEMPLATE = lib

CONFIG += staticlib

SOURCES += alpha.cpp
SOURCES += clusterfit.cpp
SOURCES += colourblock.cpp
SOURCES += colourfit.cpp
SOURCES += colourset.cpp
SOURCES += maths.cpp
SOURCES += rangefit.cpp
SOURCES += singlecolourfit.cpp
SOURCES += squish.cpp

HEADERS += alpha.h
HEADERS += clusterfit.h
HEADERS += colourblock.h
HEADERS += colourfit.h
HEADERS += colourset.h
HEADERS += config.h
HEADERS += maths.h
HEADERS += rangefit.h
HEADERS += simd_float.h
HEADERS += simd.h
HEADERS += simd_sse.h
HEADERS += simd_ve.h
HEADERS += singlecolourfit.h
HEADERS += singlecolourlookup.inl
HEADERS += squish.h

DEFINES += USE_SSE=2

INCLUDEPATH += $$PWD

include(../../library.pri)
