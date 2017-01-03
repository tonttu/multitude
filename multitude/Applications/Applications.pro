TEMPLATE = subdirs

CONFIG += Qt
QT += core

SUBDIRS += ListPortAudioDevices

stuff.path = /src/multitude/Applications
stuff.files = Applications.pro Applications.pri Applications_end.pri

INSTALLS += stuff
