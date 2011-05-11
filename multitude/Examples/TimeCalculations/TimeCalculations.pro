# Simple example, that builds an application, based on components
# already installed

include(../Examples.pri)


SOURCES += TimeCalculations.cpp

TARGET = TimeCalculations

LIBS += $$LIB_RADIANT $$LIB_PATTERNS $$LIB_NIMBLE

win32 {
  LIBS += -lSDL -lSDLmain
}
