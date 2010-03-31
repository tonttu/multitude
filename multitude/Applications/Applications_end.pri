MAINDIR=Applications
include(../app_src_inst.pri)

target.path = /bin

PROJECT_FILE = $$join(TARGET, "", "", ".pro")
srcs.path = /src/MultiTouch/multitude/Applications/$$TARGET
srcs.files = $$HEADERS $$SOURCES $$PROJECT_FILE

INSTALLS += target srcs
