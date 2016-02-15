TEMPLATE = subdirs
include(multitude.pri)
include(ThirdParty/ThirdParty.pri)

SUBDIRS += glwrapper

SUBDIRS += qjson

SUBDIRS += unittests
unittests.depends += Radiant

SUBDIRS += folly

SUBDIRS += Patterns
SUBDIRS += Nimble

SUBDIRS += Radiant
Radiant.depends = Nimble folly

SUBDIRS += Valuable
Valuable.depends = Radiant Nimble

SUBDIRS += Squish
SUBDIRS += Luminous
Luminous.depends = glwrapper Valuable

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
