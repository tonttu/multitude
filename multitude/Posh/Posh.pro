include(../multitude.pri)

HEADERS += Platform.h \
    Window.hpp \
    WindowEventHook.hpp

OTHER_FILES += Platform_OSX.mm
OTHER_FILES += WindowOSX.mm

macx {
	SOURCES += Platform_OSX.mm
	SOURCES += WindowOSX.mm
	LIBS += -framework Cocoa
}

include(../library.pri)
