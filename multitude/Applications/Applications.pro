TEMPLATE = subdirs

CONFIG += Qt
QT += core

!enable-taction:SUBDIRS += ListPortAudioDevices

CONFIG(release, debug|release) {
  stuff.path = /src/multitude/Applications
  stuff.files = Applications.pro Applications.pri Applications_end.pri

  INSTALLS += stuff
}
