TEMPLATE = lib

!isEmpty(CONFIG_LIB_PATH) {
  include($$CONFIG_LIB_PATH)
}
