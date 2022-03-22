set(LIBRARY Valuable)

project(${LIBRARY} VERSION ${PROJECT_VERSION_FULL}
  DESCRIPTION "Serialization and event library"
)

cornerstone_add_library(${LIBRARY} SHARED)

target_sources(${LIBRARY} PRIVATE
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
  SystemEvents.cpp
)

if(ENABLE_FOLLY)
  target_sources(${LIBRARY} PRIVATE EventWrapper.cpp)
endif()

if(ENABLE_PUNCTUAL)
  target_compile_definitions(${LIBRARY} PRIVATE -DENABLE_PUNCTUAL)
  target_link_libraries(${LIBRARY} PRIVATE Punctual)
endif()

target_compile_definitions(${LIBRARY} PRIVATE -DVALUABLE_EXPORT)

target_link_libraries(${LIBRARY}
  PUBLIC Radiant
)

cornerstone_add_qt_libs(${LIBRARY} PRIVATE "Xml")

cornerstone_install_lib(${LIBRARY})
