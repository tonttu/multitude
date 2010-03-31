TARGET = ExternalCompilation

SOURCES += Main.cpp

CONFIG -= qt

unix:LIBS += -lNimble -lRadiant -lVideoDisplay

macx:LIBS += -framework,Nimble -framework,Radiant -framework,VideoDisplay

CONFIG += link_pkgconfig

PKGCONFIG += libavutil


