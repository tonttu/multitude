TEMPLATE = subdirs

include(qmake_utils.prf)
include(../cornerstone.pri)

# 3rd party libraries

enable-smtp {
  smtpclient.subdir += ThirdParty/SMTPEmail
  SUBDIRS += smtpclient
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

enable-punctual {
  # Make executors separate so that this can be dependency for different
  # subcomponents (like Luminous for render threads, Valuable for after events)
  SUBDIRS += Punctual
  Punctual.depends = Radiant
}

SUBDIRS += Valuable
Valuable.depends = Radiant Nimble
enable-punctual:Valuable.depends += Punctual

enable-smtp {
  SUBDIRS += EmailSending
  EmailSending.depends += Valuable smtpclient Radiant
}

enable-luminous {
  SUBDIRS += Squish
  SUBDIRS += Luminous
  Luminous.depends = Valuable Radiant
}

enable-pdf {
  SUBDIRS += Pdf
  Pdf.depends += Punctual
  enable-luminous {
    Pdf.depends += Luminous
  }
}

enable-resonant {
  SUBDIRS += Resonant
  Resonant.depends = Radiant Nimble Valuable
}

enable-video-display {
  SUBDIRS += VideoDisplay
  VideoDisplay.depends = Resonant
}

enable-applications {
  SUBDIRS += Applications
  Applications.depends = Radiant Nimble Valuable
}
