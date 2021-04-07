include(../../cornerstone.pri)

HEADERS += \
    Export.hpp \
    TaskWrapper.hpp \
    Helpers.hpp \
    Executors.hpp \
    LimitedTimeExecutor.hpp

SOURCES += \
    Executors.cpp \
    LimitedTimeExecutor.cpp

LIBS += $$LIB_FOLLY $$LIB_RADIANT

DEFINES += PUNCTUAL_EXPORT

include(../../library.pri)
