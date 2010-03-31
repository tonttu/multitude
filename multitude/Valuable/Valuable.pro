include(../multitude.pri)

HEADERS += ChangeMap.hpp
HEADERS += CmdParser.hpp
HEADERS += ValueEnum.hpp
HEADERS += ConfigDocument.hpp
HEADERS += ConfigElement.hpp
HEADERS += ConfigValue.hpp
HEADERS += DOMDocument.hpp
HEADERS += DOMElement.hpp
HEADERS += Export.hpp
HEADERS += HasValues.hpp
HEADERS += HasValuesImpl.hpp
HEADERS += Valuable.hpp
HEADERS += ValueBool.hpp
HEADERS += ValueColor.hpp
HEADERS += ValueFloat.hpp
HEADERS += ValueFloatImpl.hpp
HEADERS += ValueInt.hpp
HEADERS += ValueIntImpl.hpp
HEADERS += ValueListener.hpp
HEADERS += ValueNumeric.hpp
HEADERS += ValueObject.hpp
HEADERS += ValueRect.hpp
HEADERS += ValueString.hpp
HEADERS += ValueStringImpl.hpp
HEADERS += ValueVector.hpp
HEADERS += ValueVectorImpl.hpp

SOURCES += ChangeMap.cpp
SOURCES += CmdParser.cpp
SOURCES += ValueEnum.cpp
SOURCES += ConfigDocument.cpp
SOURCES += ConfigElement.cpp
SOURCES += ConfigValue.cpp
SOURCES += HasValues.cpp
SOURCES += Valuable.cpp
SOURCES += ValueBool.cpp
SOURCES += ValueColor.cpp
SOURCES += ValueFloat.cpp
SOURCES += ValueInt.cpp
SOURCES += ValueListener.cpp
SOURCES += ValueObject.cpp
SOURCES += ValueRect.cpp
SOURCES += ValueString.cpp
SOURCES += ValueVector.cpp

LIBS += $$LIB_RADIANT \
    $$LIB_NIMBLE

win32:DEFINES += VALUABLE_EXPORT

contains(HAS_QT_45,YES) {
  message(Using QT XML parser)

  SOURCES += DOMDocumentQT.cpp
  SOURCES += DOMElementQT.cpp
  
  CONFIG += qt
  QT += xml
} else {
  message(Using Xerces XML parser)

  SOURCES += DOMDocumentXerces.cpp
  SOURCES += DOMElementXerces.cpp

  unix:LIBS += -lxerces-c
  win32 { 
    DEFINES += VALUABLE_EXPORT
    LIBS += xerces-c_2.lib
    QMAKE_CXXFLAGS += -Zc:wchar_t
  }
}

include(../library.pri)
