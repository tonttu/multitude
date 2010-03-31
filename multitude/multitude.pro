TEMPLATE = subdirs

include(multitude.pri)

SUBDIRS += Patterns
SUBDIRS += Nimble
SUBDIRS += Radiant
SUBDIRS += Valuable
SUBDIRS += Luminous
SUBDIRS += Poetic
SUBDIRS += Resonant
SUBDIRS += Screenplay
SUBDIRS += VideoDisplay

exists(Examples/Examples.pro):SUBDIRS += Examples

SUBDIRS += Applications

CONFIG += ordered

# Install some build files to the source package
stuff.path = /src/MultiTouch/multitude
stuff.files = LGPL.txt multitude.pro multitude.pri library.pri app_src_inst.pri

INSTALLS += stuff

win32 {
	win64:include(Win64x/Win64x.pri)
	else:include(Win32x/Win32x.pri)
}