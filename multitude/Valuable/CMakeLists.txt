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
  # With Folly
  $<$<BOOL:${ENABLE_FOLLY}>:EventWrapper.cpp>
)

target_compile_definitions(${LIBRARY} PRIVATE
  VALUABLE_EXPORT $<$<BOOL:${ENABLE_PUNCTUAL}>:ENABLE_PUNCTUAL>
)

target_link_libraries(${LIBRARY}
  PRIVATE Qt5::Xml
  PUBLIC Radiant MultiFolly $<$<BOOL:${ENABLE_PUNCTUAL}>:Punctual>
)

cornerstone_install_lib(${LIBRARY})
