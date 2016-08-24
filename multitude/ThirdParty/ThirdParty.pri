TEMPLATE = subdirs


unittests.subdir += $$PWD/unittest-cpp
folly.subdir += $$PWD/FollyFutures


# ADL SDK only has headers, we install them here manually
adl_headers.path = /src/multitude/ThirdParty/adl_sdk
adl_headers.files += adl_sdk/adl_defines.h
adl_headers.files += adl_sdk/adl_functions.h 
adl_headers.files += adl_sdk/adl_sdk.h
adl_headers.files += adl_sdk/adl_structures.h

# Also install this project file
third_party_project_file.path = /src/multitude/ThirdParty
third_party_project_file.files += ThirdParty.pri

#todo should probably install also expected etc..

INSTALLS += adl_headers third_party_project_file
