include(../multitude.pri)

HEADERS += Archive.hpp
HEADERS += AttributeFlags.hpp
HEADERS += ChangeMap.hpp
HEADERS += CmdParser.hpp
HEADERS += ConfigDocument.hpp
HEADERS += ConfigElement.hpp
HEADERS += ConfigValue.hpp
HEADERS += DOMDocument.hpp
HEADERS += DOMElement.hpp
HEADERS += Export.hpp
HEADERS += FileWatcher.hpp
HEADERS += Node.hpp
HEADERS += Serializer.hpp
HEADERS += Valuable.hpp
HEADERS += AttributeBool.hpp
HEADERS += AttributeColor.hpp
HEADERS += AttributeContainer.hpp
HEADERS += AttributeEnum.hpp
HEADERS += AttributeFloat.hpp
HEADERS += AttributeFloatImpl.hpp
HEADERS += Value.hpp
HEADERS += AttributeInt.hpp
HEADERS += AttributeIntImpl.hpp
HEADERS += AttributeMatrix.hpp
HEADERS += AttributeMatrixImpl.hpp
HEADERS += AttributeNumeric.hpp
HEADERS += AttributeObject.hpp
HEADERS += AttributeRect.hpp
HEADERS += AttributeString.hpp
HEADERS += AttributeVector.hpp
HEADERS += AttributeVectorImpl.hpp
HEADERS += XMLArchive.hpp

SOURCES += Archive.cpp
SOURCES += ChangeMap.cpp
SOURCES += CmdParser.cpp
SOURCES += ConfigDocument.cpp
SOURCES += ConfigElement.cpp
SOURCES += ConfigValue.cpp
SOURCES += DOMDocumentQT.cpp
SOURCES += DOMElementQT.cpp
SOURCES += FileWatcher.cpp
SOURCES += Node.cpp
SOURCES += Serializer.cpp
SOURCES += Valuable.cpp
SOURCES += AttributeBool.cpp
SOURCES += AttributeColor.cpp
SOURCES += AttributeEnum.cpp
SOURCES += AttributeFloat.cpp
SOURCES += AttributeInt.cpp
SOURCES += AttributeMatrix.cpp
SOURCES += AttributeObject.cpp
SOURCES += AttributeRect.cpp
SOURCES += AttributeString.cpp
SOURCES += AttributeVector.cpp
SOURCES += XMLArchive.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE $$LIB_PATTERNS

DEFINES += VALUABLE_EXPORT

CONFIG += qt
QT += xml

include(../library.pri)
