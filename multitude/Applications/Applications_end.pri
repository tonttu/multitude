target.path = /bin

CONFIG(release, debug|release) {
  srcs.path = /src/multitude/Applications/$$TARGET
  srcs.files = $$HEADERS $$SOURCES $$_PRO_FILE_

  INSTALLS += srcs
}

INSTALLS += target
