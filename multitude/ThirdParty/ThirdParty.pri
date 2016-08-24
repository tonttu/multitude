# Don't use default multitude install rules because we want these files to go
# under ThirdParty. We override this at the end of this file.
skip_install_targets = true

include(../library.pri)

# Create install targets for source code and headers
$$installFiles(/include/ThirdParty/$$TARGET_WITHOUT_VERSION, EXPORT_HEADERS)
$$installFiles(/src/multitude/ThirdParty/$$TARGET_WITHOUT_VERSION, ALL_SOURCE_CODE)
