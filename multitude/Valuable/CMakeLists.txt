include(../../common.cmake)

project(Valuable VERSION ${CORNERSTONE_VERSION} DESCRIPTION "Serialization and event library")

set(SOURCES
  Archive.cpp
  AttributeAlias.cpp
  AttributeStringList.cpp
  AttributeStringMap.cpp
  TransitionManager.cpp
  SimpleExpression.cpp
  SimpleExpressionLink.cpp
  Event.cpp
  GraphicsCoordinates.cpp
  CmdParser.cpp
  ConfigDocument.cpp
  ConfigElement.cpp
  ConfigValue.cpp
  DOMDocumentQT.cpp
  DOMElementQT.cpp
  FileWatcher.cpp
  Node.cpp
  Serializer.cpp
  StyleValue.cpp
  AttributeBool.cpp
  Attribute.cpp
  AttributeString.cpp
  XMLArchive.cpp
  State.cpp
  ListenerHolder.cpp
  AttributeSpline.cpp
  AttributeAsset.cpp
)

add_library(Valuable SHARED ${SOURCES})

add_definitions(-DVALUABLE_EXPORT)

target_link_libraries(Valuable Radiant)

if(ENABLE_FOLLY)
  list(APPEND SOURCES EventWrapper.cpp)
endif()

find_package(Qt5 COMPONENTS Xml PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

target_link_libraries(Valuable Qt5::Xml)
