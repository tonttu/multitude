TEMPLATE = subdirs

SUBDIRS += FireCapture
SUBDIRS += FireView
SUBDIRS += ListPortAudioDevices
SUBDIRS += MoviePlayer

stuff.path = /src/MultiTouch/multitude/Applications
stuff.files = Applications.pro Applications.pri Applications_end.pri

INSTALLS += stuff
