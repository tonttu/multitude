# This is for @rpath issues so the library gets the right name
macx:TARGET           = QxtNetwork

CLEAN_TARGET     = QxtNetwork
DEFINES         += BUILD_QXT_NETWORK
QT               = core network
QXT              = core
CONVENIENCE     += $$CLEAN_TARGET

include(network.pri)
include(../qxtbase.pri)
include(../../../ThirdParty.pri)
