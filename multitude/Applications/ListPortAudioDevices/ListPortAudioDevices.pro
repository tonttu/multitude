include(../Applications.pri)

SOURCES += Main.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE
LIBS += $$LIB_PATTERNS $$LIB_LUMINOUS $$LIB_VALUABLE

unix: PKGCONFIG += portaudio-2.0
CONFIG -= qt

win32 {
    
    win64 {
		INCLUDEPATH += ../../Win64x/include/portaudio
		INCLUDEPATH += ../../Win64x/include/libsndfile
		
		LIBS += -llibsndfile-1 -lportaudio -lole32 -luser32
	} else { 
		INCLUDEPATH += ../../Win32x/include/portaudio
		INCLUDEPATH += ../../Win32x/include/libsndfile
	
		LIBS += -llibsndfile-1 -lportaudio_x86
	}
}

include(../Applications_end.pri)
