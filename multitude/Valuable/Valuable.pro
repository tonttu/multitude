include(../multitude.pri)

HEADERS += Archive.hpp \
    AttributeFlags.hpp \
    FileWatcher.hpp \
    AttributeMatrix.hpp \
    AttributeMatrixImpl.hpp
HEADERS += XMLArchive.hpp
HEADERS += ChangeMap.hpp
HEADERS += CmdParser.hpp
HEADERS += AttributeEnum.hpp
HEADERS += ConfigDocument.hpp
HEADERS += ConfigElement.hpp
HEADERS += ConfigValue.hpp
HEADERS += DOMDocument.hpp
HEADERS += DOMElement.hpp
HEADERS += Export.hpp
HEADERS += Node.hpp
HEADERS += Serializer.hpp
HEADERS += Valuable.hpp
HEADERS += Value.hpp
HEADERS += AttributeBool.hpp
HEADERS += AttributeColor.hpp
HEADERS += AttributeContainer.hpp
HEADERS += AttributeFloat.hpp
HEADERS += AttributeFloatImpl.hpp
HEADERS += AttributeInt.hpp
HEADERS += AttributeIntImpl.hpp
HEADERS += AttributeNumeric.hpp
HEADERS += AttributeObject.hpp
HEADERS += AttributeRect.hpp
HEADERS += AttributeString.hpp
HEADERS += AttributeVector.hpp
HEADERS += AttributeVectorImpl.hpp

SOURCES += Archive.cpp \
    FileWatcher.cpp \
    AttributeMatrix.cpp
SOURCES += XMLArchive.cpp
SOURCES += ChangeMap.cpp
SOURCES += CmdParser.cpp
SOURCES += AttributeEnum.cpp
SOURCES += ConfigDocument.cpp
SOURCES += ConfigElement.cpp
SOURCES += ConfigValue.cpp
SOURCES += Node.cpp
SOURCES += Serializer.cpp
SOURCES += Valuable.cpp
SOURCES += AttributeBool.cpp
SOURCES += AttributeColor.cpp
SOURCES += AttributeFloat.cpp
SOURCES += AttributeInt.cpp
SOURCES += AttributeObject.cpp
SOURCES += AttributeRect.cpp
SOURCES += AttributeString.cpp
SOURCES += AttributeVector.cpp

LIBS += $$LIB_RADIANT \
    $$LIB_NIMBLE $$LIB_PATTERNS -lv8

win32:DEFINES += VALUABLE_EXPORT

contains(HAS_QT_45,YES) {
  message(Using QT XML parser)

  SOURCES += DOMDocumentQT.cpp
  SOURCES += DOMElementQT.cpp

  CONFIG += qt
  QT += xml
}

include(../library.pri)
