# Simple example, that builds an application, based on components
# already installed

SOURCES += Main.cpp

TARGET = BuildExample

unix:LIBS += -lNimble

macx:LIBS += -framework,Nimble

win32 {
	include(../../Win32/WinApps.pri)
	INCLUDEPATH += "../.." $$INC_WINPORT $$INC_GLEW $$INC_PTHREADS $$INC_FFMPEG $$INC_XERCES
	QMAKE_CXXFLAGS *= -wd4251	# see http://www.unknownroad.com/rtfm/VisualStudio/warningC4251.html
}
