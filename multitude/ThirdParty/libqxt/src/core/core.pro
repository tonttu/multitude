include(../../../../../cornerstone.pri)

# This is for @rpath issues so the library gets the right name
macx:TARGET           = QxtCore

CLEAN_TARGET     = QxtCore
DEFINES         += BUILD_QXT_CORE
QT               = core
QXT              =
CONVENIENCE     += $$CLEAN_TARGET

*clang* | *g++*: QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-variable

include(core.pri)
include(../qxtbase.pri)
include(../../../ThirdParty.pri)
