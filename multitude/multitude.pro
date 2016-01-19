TEMPLATE = subdirs
include(multitude.pri)
CONFIG += ordered

SUBDIRS += ThirdParty
SUBDIRS += Patterns
SUBDIRS += Nimble
SUBDIRS += Radiant
SUBDIRS += Valuable
SUBDIRS += Squish
SUBDIRS += Luminous
SUBDIRS += Resonant
SUBDIRS += VideoDisplay

enable-extras {
  SUBDIRS += Applications
}

# Install some build files to the source package
stuff.path = /src/multitude
stuff.files = LGPL.txt multitude.pro multitude.pri library.pri

INSTALLS += stuff

# Install extra dependencies on Windows
win*:include(Win64x/Win64x.pri)
