# Expected is a header only library, so just copy everything as is.
expected_includes.path = /include/ThirdParty
expected_includes.files = ThirdParty/expected

CONFIG(release, debug|release) {
  enable-sdk {
    INSTALLS += expected_includes
  }
}
