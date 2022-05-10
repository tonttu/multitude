!exists($${CONFIG_PATH}) {
  error("Could not find external qmake settings file " $${CONFIG_PATH});
}

include($${CONFIG_PATH})

include(multitude-core.pri)
