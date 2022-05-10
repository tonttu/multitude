include(../../cornerstone.pri)

QT += network

HEADERS += Email.hpp \
    Sender.hpp \
    Export.hpp \
    SendImplementation.hpp


SOURCES += Email.cpp \
    Sender.cpp \
    SendImplementation.cpp

DEFINES += EMAIL_EXPORT

LIBS += $$LIB_VALUABLE $$LIB_FOLLY $$LIB_RADIANT $$LIB_SMTP_CLIENT

include(../../library.pri)
