TEMPLATE = subdirs

include(qmake_utils.prf)
include(../cornerstone.pri)

# 3rd party libraries

smtpclient.subdir += ThirdParty/SMTPEmail
SUBDIRS += smtpclient

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

SUBDIRS += EmailSending
EmailSending.depends += Valuable folly smtpclient Radiant

SUBDIRS += Squish
SUBDIRS += Luminous
Luminous.depends = Valuable Punctual folly Radiant

SUBDIRS += Resonant
Resonant.depends = Radiant Nimble Valuable

enable-video-display {
  SUBDIRS += VideoDisplay
  VideoDisplay.depends = Resonant Luminous
}

enable-applications {
  SUBDIRS += Applications
  Applications.depends = Radiant Nimble Valuable
}
