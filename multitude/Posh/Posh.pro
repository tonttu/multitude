include(../multitude.pri)

HEADERS += Platform.h

OTHER_FILES += Platform_OSX.mm

macx {
	SOURCES += Platform_OSX.mm
	LIBS += -framework Cocoa
}

include(../library.pri)
