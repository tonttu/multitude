include(../Examples.pri)
SOURCES += Main.cpp
LIBS += $$LIB_RADIANT \
    $$LIB_RESONANT \
    $$LIB_VALUABLE \
    $$LIB_PATTERNS
win32 {
    INCLUDEPATH += $$INC_WINPORT
    QMAKE_LIBDIR += $$LNK_MULTITUDE
    LIBS += ws2_32.lib
}
unix: PKGCONFIG += sndfile

# HEADERS += ../../../Tests/CameraWidget/VideoAnnotations.hpp
