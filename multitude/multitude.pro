TEMPLATE = subdirs

include(multitude.pri)
include(Externals.pri)

enable-js:SUBDIRS += v8
SUBDIRS += Patterns
SUBDIRS += Nimble
SUBDIRS += Radiant
SUBDIRS += Valuable
!mobile*:SUBDIRS += Squish
SUBDIRS += Luminous
SUBDIRS += Poetic
SUBDIRS += Resonant
!mobile*:SUBDIRS += Screenplay
!mobile*:SUBDIRS += VideoDisplay
SUBDIRS += Box2D
#SUBDIRS += Posh


vivid {
  SUBDIRS += Vivid
}

#exists(Examples/Examples.pro):SUBDIRS += Examples
!mobile*{
#  SUBDIRS += Applications
}

CONFIG += ordered

# Install some build files to the source package
stuff.path = /src/MultiTouch/multitude
stuff.files = LGPL.txt multitude.pro multitude.pri library.pri

INSTALLS += stuff

win32 {
    win64:include(Win64x/Win64x.pri)
    else:include(Win32x/Win32x.pri)
}

# message(Config is $${CONFIG})
