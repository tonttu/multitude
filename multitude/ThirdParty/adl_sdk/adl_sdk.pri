# ADL SDK is a header-only library. Just copy everything as is.
adl_includes.path = /include/ThirdParty
adl_includes.files = ThirdParty/adl_sdk

adl_sources.path = /src/multitude/ThirdParty
adl_sources.files = ThirdParty/adl_sdk

CONFIG(release, debug|release) {
  INSTALLS += adl_includes adl_sources
}
