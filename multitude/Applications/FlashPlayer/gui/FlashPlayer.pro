CONFIG += qt
QT += gui xml
HEADERS += FlashPlayer.hpp \
    Options.hpp
SOURCES += FlashPlayer.cpp \
    Options.cpp
LIBS += -lXinerama

FORMS += \
    Options.ui
	
# Apparently qmake doesn't automagically add FORMS to the install target
forms.path = /src/MultiTouch/Applications/FlashPlayer/gui
forms.files = $$FORMS
INSTALLS += forms

RESOURCES += \
    resources.qrc
