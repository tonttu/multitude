include(../multitude.pri)

HEADERS += \
    TaskScheduler.hpp \
    Export.hpp \
    TaskWrapper.hpp

SOURCES += \
    TaskScheduler.cpp

LIBS += $$LIB_FOLLY_FUTURES $$LIB_RADIANT

DEFINES += PUNCTUAL_EXPORT

include(../library.pri)
