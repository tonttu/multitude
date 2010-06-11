CONFIG += qt
QT += gui xml
HEADERS += FlashPlayer.hpp \
    Options.hpp
SOURCES += FlashPlayer.cpp \
    Options.cpp
LIBS += -lXinerama

FORMS += \
    Options.ui
