include(../Applications.pri)

SOURCES += Main.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE
LIBS += $$LIB_PATTERNS $$LIB_LUMINOUS $$LIB_VALUABLE $$LIB_V8

unix: PKGCONFIG += portaudio-2.0
CONFIG -= qt

win32 {
    
  win64 {
		INCLUDEPATH += ../../Win64x/include/portaudio
		INCLUDEPATH += ../../Win64x/include/libsndfile

    QMAKE_LIBDIR += $$DDK_PATH\\lib\\win7\\amd64
		LIBS += -llibsndfile-1 -lportaudio -lole32 -luser32
	} else { 
		INCLUDEPATH += ../../Win32x/include/portaudio
		INCLUDEPATH += ../../Win32x/include/libsndfile
	
    QMAKE_LIBDIR += $$DDK_PATH\\lib\\win7\\i386
		LIBS += -llibsndfile-1 -lportaudio_x86
	}
}

include(../Applications_end.pri)
