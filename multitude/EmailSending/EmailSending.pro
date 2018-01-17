include(../../cornerstone.pri)

QT += network

HEADERS += Email.hpp \
    Sender.hpp \
    Export.hpp \
    SendImplementation.hpp


SOURCES += Email.cpp \
    Sender.cpp \
    SendImplementation.cpp

LIBS += $$LIB_VALUABLE $$LIB_FOLLY_FUTURES $$LIB_RADIANT $$LIB_SMTP_CLIENT

include(../../library.pri)
