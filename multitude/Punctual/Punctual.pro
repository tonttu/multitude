include(../../cornerstone.pri)

HEADERS += \
    TaskScheduler.hpp \
    Export.hpp \
    TaskWrapper.hpp

SOURCES += \
    TaskScheduler.cpp

LIBS += $$LIB_FOLLY_FUTURES $$LIB_RADIANT

DEFINES += PUNCTUAL_EXPORT

gcc:QMAKE_CXXFLAGS_WARN_ON += -Werror

include(../library.pri)
