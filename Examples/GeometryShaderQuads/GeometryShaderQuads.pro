include(../Examples.pri)
SOURCES += GeometryShaderQuads.cpp
unix { 
    CONFIG += link_pkgconfig
    PKGCONFIG += sdl
}
LIBS += $$LIB_RADIANT \
    $$LIB_PATTERNS \
    $$LIB_LUMINOUS \
    $$LIB_VALUABLE \
    $$LIB_OPENGL \
    $$LIB_GLEW \
    $$LIB_NIMBLE
win32:LIBS += -lSDL \
    -lSDLmain
OTHER_FILES += shader-quads.gs \
    shader-quads.vs \
    shader-quads.ps
