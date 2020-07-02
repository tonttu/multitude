include(../../cornerstone.pri)

HEADERS += \
    Export.hpp \
    TaskWrapper.hpp \
    Helpers.hpp \
    Executors.hpp \
    OnDemandExecutor.hpp

SOURCES += \
    Executors.cpp \
    OnDemandExecutor.cpp

LIBS += $$LIB_FOLLY $$LIB_RADIANT

DEFINES += PUNCTUAL_EXPORT

include(../../library.pri)
