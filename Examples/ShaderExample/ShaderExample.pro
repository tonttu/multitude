include(../Examples.pri)

SOURCES += Main.cpp

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += sdl
}

LIBS += $$LIB_RADIANT $$LIB_PATTERNS $$LIB_LUMINOUS $$LIB_VALUABLE $$LIB_OPENGL $$LIB_NIMBLE

LIBS += $$LIB_GLEW -lSDL -lSDLmain
