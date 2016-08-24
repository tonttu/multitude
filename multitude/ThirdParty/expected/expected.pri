# Expected is a header only library, so just copy everything as is.
expected_includes.path = /include/ThirdParty
expected_includes.files = ThirdParty/expected

expected_sources.path = /src/multitude/ThirdParty
expected_sources.files = ThirdParty/expected

INSTALLS += expected_includes expected_sources
