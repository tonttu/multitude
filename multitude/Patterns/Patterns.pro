include(../../cornerstone.pri)

HEADERS += Export.hpp
HEADERS += NotCopyable.hpp
HEADERS += Patterns.hpp

SOURCES += NotCopyable.cpp

win32:DEFINES += PATTERNS_EXPORT

include(../library.pri)
