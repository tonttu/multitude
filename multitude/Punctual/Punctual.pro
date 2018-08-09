include(../../cornerstone.pri)

HEADERS += \
    TaskScheduler.hpp \
    Export.hpp \
    TaskWrapper.hpp

SOURCES += \
    TaskScheduler.cpp

LIBS += $$LIB_FOLLY $$LIB_RADIANT

DEFINES += PUNCTUAL_EXPORT

include(../../library.pri)
