# Universal settings for multitude compilations

INCLUDEPATH += $$PWD $$PWD/ThirdParty

android {
  CONFIG += mobile
  DEFINES += RADIANT_MOBILE
}
