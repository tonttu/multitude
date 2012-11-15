TEMPLATE = subdirs

include(multitude.pri)
include(Externals.pri)

!macx:SUBDIRS += ThirdParty/glew
SUBDIRS += Patterns
SUBDIRS += Nimble
SUBDIRS += Radiant
SUBDIRS += Valuable
!mobile*:SUBDIRS += Squish
SUBDIRS += Luminous
SUBDIRS += Resonant
SUBDIRS += VideoDisplay
SUBDIRS += Box2D

#exists(Examples/Examples.pro):SUBDIRS += Examples
!mobile*{
#  SUBDIRS += Applications
}

CONFIG += ordered

# Install some build files to the source package
stuff.path = /src/multitude
stuff.files = LGPL.txt multitude.pro multitude.pri library.pri

INSTALLS += stuff

win32 {
    win64:include(Win64x/Win64x.pri)
    else:include(Win32x/Win32x.pri)
}
