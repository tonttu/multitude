include(../multitude.pri)

HEADERS += Export.hpp
HEADERS += NotCopyable.hpp
HEADERS += Singleton.hpp

SOURCES += Dummy.cpp
SOURCES += NotCopyable.cpp

win32:DEFINES += PATTERNS_EXPORT

include(../library.pri)
