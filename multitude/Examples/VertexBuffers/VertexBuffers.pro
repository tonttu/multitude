include(../Examples.pri)

SOURCES += Main.cpp

unix {
	CONFIG += link_pkgconfig
	PKGCONFIG += sdl
}

LIBS += $$LIB_LUMINOUS $$LIB_VALUABLE $$LIB_RADIANT $$LIB_OPENGL  $$LIB_PATTERNS $$LIB_NIMBLE

win32 {
	LIBS += -lSDL -lSDLmain
}
