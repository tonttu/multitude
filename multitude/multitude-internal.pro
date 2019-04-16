TEMPLATE = subdirs

unittestcpp.subdir += ThirdParty/UnitTest++
unittestcpp.depends += Radiant
SUBDIRS += unittestcpp

SUBDIRS += Patterns
SUBDIRS += Nimble

SUBDIRS += Radiant
Radiant.depends = Nimble

SUBDIRS += Valuable
Valuable.depends = Radiant Nimble

enable-resonant {
  SUBDIRS += Resonant
  Resonant.depends = Radiant Nimble Valuable
}


# sudo apt-get install libudev-dev
