include(../multitude.pri)

HEADERS += Archive.hpp
HEADERS += ChangeMap.hpp
HEADERS += CmdParser.hpp
HEADERS += ConfigDocument.hpp
HEADERS += ConfigElement.hpp
HEADERS += ConfigValue.hpp
HEADERS += DOMDocument.hpp
HEADERS += DOMElement.hpp
HEADERS += Export.hpp
HEADERS += HasValues.hpp
HEADERS += HasValuesImpl.hpp
HEADERS += Serializer.hpp
HEADERS += Valuable.hpp
HEADERS += ValueBool.hpp
HEADERS += ValueColor.hpp
HEADERS += ValueContainer.hpp
HEADERS += ValueEnum.hpp
HEADERS += ValueFloat.hpp
HEADERS += ValueFloatImpl.hpp
HEADERS += Value.hpp
HEADERS += ValueInt.hpp
HEADERS += ValueIntImpl.hpp
HEADERS += ValueListener.hpp
HEADERS += ValueMatrix.hpp
HEADERS += ValueMatrixImpl.hpp
HEADERS += ValueNumeric.hpp
HEADERS += ValueObject.hpp
HEADERS += ValueRect.hpp
HEADERS += ValueString.hpp
HEADERS += ValueStringImpl.hpp
HEADERS += ValueVector.hpp
HEADERS += ValueVectorImpl.hpp
HEADERS += XMLArchive.hpp

SOURCES += Archive.cpp
SOURCES += ChangeMap.cpp
SOURCES += CmdParser.cpp
SOURCES += ConfigDocument.cpp
SOURCES += ConfigElement.cpp
SOURCES += ConfigValue.cpp
SOURCES += DOMDocumentQT.cpp
SOURCES += DOMElementQT.cpp
SOURCES += HasValues.cpp
SOURCES += Serializer.cpp
SOURCES += Valuable.cpp
SOURCES += ValueBool.cpp
SOURCES += ValueColor.cpp
SOURCES += ValueEnum.cpp
SOURCES += ValueFloat.cpp
SOURCES += ValueInt.cpp
SOURCES += ValueListener.cpp
SOURCES += ValueMatrix.cpp
SOURCES += ValueObject.cpp
SOURCES += ValueRect.cpp
SOURCES += ValueString.cpp
SOURCES += ValueVector.cpp
SOURCES += XMLArchive.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE $$LIB_PATTERNS

DEFINES += VALUABLE_EXPORT

CONFIG += qt
QT += xml

include(../library.pri)
