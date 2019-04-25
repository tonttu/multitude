TEMPLATE = lib

!isEmpty(CONFIG_LIB_PATH) {
  message("Including lib config")
  include($$CONFIG_LIB_PATH)
}
