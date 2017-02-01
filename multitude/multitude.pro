TEMPLATE = subdirs

include(qmake_utils.prf)
include(../cornerstone.pri)

# 3rd party libraries

!arm64 {
  libqxt.subdir += ThirdParty/libqxt
  SUBDIRS += libqxt
}

folly.subdir += ThirdParty/folly
SUBDIRS += folly

unittestcpp.subdir += ThirdParty/UnitTest++
unittestcpp.depends += Radiant
SUBDIRS += unittestcpp

include(ThirdParty/adl_sdk/adl_sdk.pri)
include(ThirdParty/expected/expected.pri)

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
Luminous.depends = Valuable Punctual folly Radiant

SUBDIRS += Resonant
Resonant.depends = Radiant Nimble Valuable

SUBDIRS += VideoDisplay
VideoDisplay.depends = Resonant Luminous

enable-extras {
  SUBDIRS += Applications
  Applications.depends = Radiant Nimble Valuable
}

# Install some build files to the source package
MISC_FILES += LGPL.txt multitude.pro multitude.pri library.pri qmake_utils.prf
MISC_FILES += ThirdParty/ThirdParty.pri

$$installFiles(/src/multitude, MISC_FILES)

# Install extra dependencies on Windows
win*:include(Win64x/Win64x.pri)
