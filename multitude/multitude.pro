TEMPLATE = subdirs
include(multitude.pri)

SUBDIRS += ThirdParty
SUBDIRS += Patterns
SUBDIRS += Nimble

SUBDIRS += Radiant
Radiant.depends = Nimble ThirdParty

SUBDIRS += Valuable
Valuable.depends = Radiant Nimble

SUBDIRS += Squish
SUBDIRS += Luminous
Luminous.depends = ThirdParty Valuable

SUBDIRS += Resonant
Resonant.depends = Radiant Nimble Valuable

SUBDIRS += VideoDisplay
VideoDisplay.depends = Resonant Luminous

enable-extras {
  SUBDIRS += Applications
  Applications.depends = Radiant Nimble Luminous
}

# Install some build files to the source package
stuff.path = /src/multitude
stuff.files = LGPL.txt multitude.pro multitude.pri library.pri

INSTALLS += stuff

# Install extra dependencies on Windows
win*:include(Win64x/Win64x.pri)
