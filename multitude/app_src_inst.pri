
macx {

  APPDIR=$$MAINDIR/$$TARGET/
  APPSRCDIR=$$MAINDIR/$$TARGET/$${TARGET}.app/Contents/Source

#  message($$system(pwd))
  message(Putting sources to: $$APPSRCDIR)
  system(install -d $$APPSRCDIR)
  system(cp $$APPDIR/*.[hc]pp $$APPSRCDIR/)

  !isEmpty(APP_EXTRA_SRC) {
    message(With extra application src = $$APP_EXTRA_SRC)
    system(cp $$APPDIR/$$APP_EXTRA_SRC $$APPSRCDIR/)
  }
}
