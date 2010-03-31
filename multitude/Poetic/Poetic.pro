include(../multitude.pri)

HEADERS += BBox.hpp
HEADERS += Charmap.hpp
HEADERS += CPUBitmapFont.hpp
HEADERS += CPUBitmapGlyph.hpp
HEADERS += CPUFontBase.hpp
HEADERS += CPUFont.hpp
HEADERS += CPUManagedFont.hpp
HEADERS += CPUWrapperFont.hpp
HEADERS += Poetic.hpp
HEADERS += Export.hpp
HEADERS += Face.hpp
HEADERS += FontManager.hpp
HEADERS += GlyphContainer.hpp
HEADERS += Glyph.hpp
HEADERS += GPUFontBase.hpp
HEADERS += GPUFont.hpp
HEADERS += GPUManagedFont.hpp
HEADERS += GPUTextureFont.hpp
HEADERS += GPUTextureGlyph.hpp
HEADERS += GPUWrapperFont.hpp
HEADERS += Size.hpp
HEADERS += Utils.hpp

SOURCES += BBox.cpp
SOURCES += Charmap.cpp
SOURCES += CPUBitmapFont.cpp
SOURCES += CPUBitmapGlyph.cpp
SOURCES += CPUFont.cpp
SOURCES += CPUFontBase.cpp
SOURCES += CPUManagedFont.cpp
SOURCES += CPUWrapperFont.cpp
SOURCES += Poetic.cpp
SOURCES += Face.cpp
SOURCES += FontManager.cpp
SOURCES += GlyphContainer.cpp
SOURCES += Glyph.cpp
SOURCES += GPUFontBase.cpp
SOURCES += GPUFont.cpp
SOURCES += GPUManagedFont.cpp
SOURCES += GPUTextureFont.cpp
SOURCES += GPUTextureGlyph.cpp
SOURCES += GPUWrapperFont.cpp
SOURCES += Size.cpp
SOURCES += Utils.cpp

DEFINES += POETIC_FLIP_Y

unix: PKGCONFIG += freetype2

LIBS += $$LIB_LUMINOUS $$LIB_RADIANT $$LIB_NIMBLE $$LIB_PATTERNS
LIBS += $$LIB_OPENGL

macx:LIBS += -framework,OpenGL

include(../library.pri)

win32 {
	DEFINES += POETIC_EXPORT

	win64 {
		INCLUDEPATH += ..\Win64x\include\freetype2
		LIBS += -lfreetype2
	} else {
		INCLUDEPATH += ..\Win32x\include\freetype2
		LIBS += -lfreetype
	}
}
