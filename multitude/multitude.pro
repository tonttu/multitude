TEMPLATE = subdirs
include(multitude.pri)
include(ThirdParty/ThirdParty.pri)

SUBDIRS += qjson

SUBDIRS += unittests
unittests.depends += Radiant

SUBDIRS += folly

SUBDIRS += Patterns
SUBDIRS += Nimble

SUBDIRS += Radiant
Radiant.depends = Nimble folly

# Make executors separate so that this can be dependency for different
# subcomponents (like Luminous for render threads, Valuable for after events)
SUBDIRS += Punctual
Punctual.depends = folly Radiant

SUBDIRS += Valuable
Valuable.depends = Radiant Nimble Punctual folly

SUBDIRS += Squish
SUBDIRS += Luminous
Luminous.depends = Valuable

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
