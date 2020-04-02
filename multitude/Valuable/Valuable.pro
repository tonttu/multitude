include(../setup.pri)

linux-* {
    QMAKE_CXXFLAGS += -faligned-new -Wno-error=attributes
}

HEADERS += Archive.hpp \
    AttributeFrame.hpp \
    AttributeLocation.hpp \
    AttributeAlias.hpp \
    AttributeTimeStamp.hpp \
    AttributeStringList.hpp \
    AttributeStyleValue.hpp \
    AttributeSize.hpp \
    AttributeStringMap.hpp \
    TransitionManager.hpp \
    TransitionImpl.hpp \
    TransitionAnim.hpp \
    SimpleExpression.hpp \
    SimpleExpressionLink.hpp \
    AttributeTuple.hpp \
    AttributeVectorContainer.hpp \
    AttributeEvent.hpp \
    GraphicsCoordinates.hpp \
    WeakAttributePtr.hpp \
    WeakNodePtr.hpp
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
HEADERS += NodeUtils.hpp
HEADERS += Serializer.hpp
HEADERS += StyleValue.hpp
HEADERS += Valuable.hpp
HEADERS += AttributeBool.hpp
HEADERS += AttributeColor.hpp
HEADERS += AttributeContainer.hpp
HEADERS += AttributeEnum.hpp
HEADERS += AttributeFloat.hpp
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
HEADERS += ListenerHolder.hpp
HEADERS += AttributeSpline.hpp
HEADERS += AttributeAsset.hpp
HEADERS += Event.hpp
HEADERS += EventImpl.hpp
HEADERS += Reference.hpp

SOURCES += Archive.cpp \
    AttributeAlias.cpp \
    AttributeStringList.cpp \
    AttributeStringMap.cpp \
    TransitionManager.cpp \
    SimpleExpression.cpp \
    SimpleExpressionLink.cpp \
    AttributeEvent.cpp \
    GraphicsCoordinates.cpp
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
SOURCES += ListenerHolder.cpp
SOURCES += AttributeSpline.cpp
SOURCES += AttributeAsset.cpp

LIBS += $$LIB_RADIANT $$LIB_NIMBLE $$LIB_PATTERNS $$LIB_PUNCTUAL

DEFINES += VALUABLE_EXPORT

enable-punctual:DEFINES += ENABLE_PUNCTUAL

enable-folly {
  HEADERS += EventWrapper.hpp
  SOURCES += EventWrapper.cpp
  LIBS += $$LIB_FOLLY
}

CONFIG += qt
QT += xml

include(../setup-lib.pri)

