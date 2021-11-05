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
  AttributeEvent.cpp
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
  Symbol.cpp
)

if(ENABLE_FOLLY)
  list(APPEND SOURCES EventWrapper.cpp)
endif()

add_library(Valuable SHARED ${SOURCES})

target_compile_definitions(Valuable PRIVATE -DVALUABLE_EXPORT)

target_link_libraries(Valuable PUBLIC Radiant)

find_package(Qt5 COMPONENTS Xml PATHS ${QT_PATH} REQUIRED NO_DEFAULT_PATH)

target_link_libraries(Valuable PRIVATE Qt5::Xml)
