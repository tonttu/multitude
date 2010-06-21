include(../multitude.pri)

INCLUDEPATH += ../../
DEPENDPATH += ../../

LIBS += $${MULTI_LIB_FLAG}../../lib 

win32:CONFIG += console embed_manifest_exe
