include(../../cornerstone.pri)

HEADERS += \
    Export.hpp \
    TaskWrapper.hpp \
    Helpers.hpp \
    Executors.hpp

SOURCES += \
    Executors.cpp

LIBS += $$LIB_FOLLY $$LIB_RADIANT

DEFINES += PUNCTUAL_EXPORT

include(../../library.pri)
