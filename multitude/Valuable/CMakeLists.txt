include(../../common.cmake)

set(LIBRARY Valuable)

project(${LIBRARY} VERSION ${CORNERSTONE_VERSION}
  DESCRIPTION "Serialization and event library"
)

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

cornerstone_add_library(${LIBRARY} SHARED ${SOURCES})

target_compile_definitions(${LIBRARY} PRIVATE -DVALUABLE_EXPORT)

target_link_libraries(${LIBRARY}
  PUBLIC Radiant
)

cornerstone_add_qt_libs(${LIBRARY} PRIVATE "Xml")

cornerstone_install_lib(${LIBRARY})
