include(../multitude.pri)

HEADERS += Archive.hpp \
    AttributeFrame.hpp \
    AttributeLocation.hpp \
    AttributeAlias.hpp \
    AttributeTimeStamp.hpp \
    AttributeStringList.hpp \
    AttributeStyleValue.hpp \
    AttributeSize.hpp \
    AttributeStringMap.hpp
HEADERS += AttributeFlags.hpp
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
HEADERS += StyleValue.hpp
HEADERS += Valuable.hpp
HEADERS += AttributeBool.hpp
HEADERS += AttributeColor.hpp
HEADERS += AttributeContainer.hpp
HEADERS += AttributeEnum.hpp
HEADERS += AttributeFloat.hpp
HEADERS += Value.hpp
HEADERS += AttributeInt.hpp
HEADERS += AttributeMatrix.hpp
HEADERS += AttributeNumeric.hpp
HEADERS += Attribute.hpp
HEADERS += AttributeRect.hpp
HEADERS += AttributeString.hpp
HEADERS += AttributeVector.hpp
HEADERS += AttributeGrid.hpp
HEADERS += XMLArchive.hpp
HEADERS += State.hpp
HEADERS += v8.hpp
HEADERS += ListenerHolder.hpp
HEADERS += AttributeSpline.hpp

SOURCES += Archive.cpp \
    AttributeAlias.cpp \
    AttributeStringList.cpp \
    AttributeStringMap.cpp \
    ListenerHolder.cpp
SOURCES += CmdParser.cpp
SOURCES += ConfigDocument.cpp
SOURCES += ConfigElement.cpp
SOURCES += ConfigValue.cpp
SOURCES += DOMDocumentQT.cpp
SOURCES += DOMElementQT.cpp
SOURCES += FileWatcher.cpp
SOURCES += Node.cpp
SOURCES += Serializer.cpp
SOURCES += StyleValue.cpp
SOURCES += AttributeBool.cpp
SOURCES += Attribute.cpp
SOURCES += AttributeString.cpp
SOURCES += XMLArchive.cpp
SOURCES += State.cpp
SOURCES += AttributeSpline.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE $$LIB_PATTERNS $$LIB_V8

DEFINES += VALUABLE_EXPORT

CONFIG += qt
QT += xml

include(../library.pri)
