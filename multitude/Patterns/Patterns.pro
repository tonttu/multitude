include(../multitude.pri)

HEADERS += Export.hpp
HEADERS += Factory.hpp
HEADERS += NotCopyable.hpp
HEADERS += Singleton.hpp

SOURCES += NotCopyable.cpp

win32:DEFINES += PATTERNS_EXPORT

include(../library.pri)
