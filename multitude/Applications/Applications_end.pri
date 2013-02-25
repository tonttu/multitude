target.path = /bin

PROJECT_FILE = $$join(TARGET, "", "", ".pro")
srcs.path = /src/multitude/Applications/$$TARGET
srcs.files = $$HEADERS $$SOURCES $$PROJECT_FILE

INSTALLS += target srcs
