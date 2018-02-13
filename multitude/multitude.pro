TEMPLATE = subdirs

include(qmake_utils.prf)
include(../cornerstone.pri)

# 3rd party libraries

enable-smtp {
  smtpclient.subdir += ThirdParty/SMTPEmail
  SUBDIRS += smtpclient
}

enable-folly {
  folly.subdir += ThirdParty/folly
  SUBDIRS += folly
}

unittestcpp.subdir += ThirdParty/UnitTest++
unittestcpp.depends += Radiant
SUBDIRS += unittestcpp

include(ThirdParty/adl_sdk/adl_sdk.pri)
include(ThirdParty/expected/expected.pri)

SUBDIRS += Patterns
SUBDIRS += Nimble

SUBDIRS += Radiant
Radiant.depends = Nimble
enable-folly:Radiant.depends += folly

enable-punctual {
  # Make executors separate so that this can be dependency for different
  # subcomponents (like Luminous for render threads, Valuable for after events)
  SUBDIRS += Punctual
  Punctual.depends = folly Radiant
}

SUBDIRS += Valuable
Valuable.depends = Radiant Nimble
enable-punctual:Valuable.depends += Punctual folly

enable-smtp {
  SUBDIRS += EmailSending
  EmailSending.depends += Valuable folly smtpclient Radiant
}

SUBDIRS += Squish
enable-luminous {
  SUBDIRS += Luminous
  Luminous.depends = Valuable Radiant
  enable-pdf {
    Luminous.depends += Punctual folly
  }
}

SUBDIRS += Resonant
Resonant.depends = Radiant Nimble Valuable

enable-video-display {
  SUBDIRS += VideoDisplay
  VideoDisplay.depends = Resonant
}

enable-applications {
  SUBDIRS += Applications
  Applications.depends = Radiant Nimble Valuable
}
