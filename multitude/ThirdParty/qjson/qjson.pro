include(../../multitude.pri)
TARGET   = qjson
include(../../library.pri)

SRC = ./src

TEMPLATE = lib
QT      -= gui

DEFINES += QJSON_MAKEDLL

INCLUDEPATH += $$SRC

PRIVATE_HEADERS += \
  $$SRC/json_parser.hh \
  $$SRC/json_scanner.h \
  $$SRC/location.hh \
  $$SRC/parser_p.h  \
  $$SRC/position.hh \
  $$SRC/qjson_debug.h  \
  $$SRC/stack.hh \
  $$SRC/FlexLexer.h

PUBLIC_HEADERS += \
  $$SRC/parser.h \
  $$SRC/parserrunnable.h \
  $$SRC/qobjecthelper.h \
  $$SRC/serializer.h \
  $$SRC/serializerrunnable.h \
  $$SRC/qjson_export.h

HEADERS += $$PRIVATE_HEADERS $$PUBLIC_HEADERS

SOURCES += \
  $$SRC/json_parser.cc \
  $$SRC/json_scanner.cc \
  $$SRC/json_scanner.cpp \
  $$SRC/parser.cpp \
  $$SRC/parserrunnable.cpp \
  $$SRC/qobjecthelper.cpp \
  $$SRC/serializer.cpp \
  $$SRC/serializerrunnable.cpp
