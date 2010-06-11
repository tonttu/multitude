TARGET = ExternalCompilation

SOURCES += Main.cpp

CONFIG -= qt

unix:LIBS += -lNimble -lRadiant -lVideoDisplay

macx:LIBS += -framework,Nimble -framework,Radiant -framework,VideoDisplay

unix:LIBS += $$MULTI_FFMPEG_LIBS

