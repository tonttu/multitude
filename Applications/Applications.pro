# Include is needed for enable-port-audio-check
include(../../Applications/Applications.pri)

TEMPLATE = subdirs

CONFIG += Qt
QT += core

enable-port-audio:SUBDIRS += ListPortAudioDevices
