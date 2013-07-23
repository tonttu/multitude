# Common rules to build libraries
TEMPLATE = lib

# This will disable generation of .so version symlinks on Linux
unix:CONFIG += plugin

PROJECT_FILE = $$_PRO_FILE_

# Make sure we don't override this if it has been set already
isEmpty(DESTDIR):DESTDIR = $$PWD/lib

isEmpty(EXPORT_HEADERS):EXPORT_HEADERS = $$HEADERS
isEmpty(EXPORT_SOURCES):EXPORT_SOURCES = $$SOURCES
contains(EXPORT_SOURCES, nothing) {
  EXPORT_SOURCES=
  PROJECT_FILE=
}

# Default location to install libraries (override in platform specific sections
# below)
target.path = /lib

# Installation target for header files
includes.path = /include/$$TARGET
includes.files = $$EXPORT_HEADERS

# Installation target for generated header files (bison, flex)
extra_inc.path = /include/$$TARGET
extra_inc.files = $$EXTRA_HEADERS
extra_inc.CONFIG += no_check_exist

# Installation target for source code
src_code.path = /src/multitude/$$TARGET
src_code.files += $$EXPORT_SOURCES $$EXPORT_HEADERS
src_code.files += $$FLEXSOURCES $$BISONSOURCES
src_code.files += $$PROJECT_FILE

INSTALLS += target
INSTALLS += includes src_code extra_inc

!no_version_in_target:TARGET=$$join(TARGET,,,$${CORNERSTONE_LIB_SUFFIX})

# On Windows, put DLLs into /bin with the exes
win32 {
  DLLDESTDIR = $$PWD/bin

  # Optimized debug libraries
  CONFIG(debug,debug|release) {
    CONFIG(optimized) {
      # Set optimization level
      QMAKE_CFLAGS_DEBUG += -O2
      QMAKE_CXXFLAGS_DEBUG += -O2
      QMAKE_CXXFLAGS_DEBUG=$$replace(QMAKE_CXXFLAGS_DEBUG,-Zi,)
      QMAKE_LFLAGS_DEBUG=$$replace(QMAKE_LFLAGS_DEBUG,/DEBUG,)
    }
  }

  # Install DLL to binary folder
  dlls.path = /bin
  dlls.files = $${DESTDIR}/$${TARGET}.dll
  dlls.CONFIG += no_check_exist

  INSTALLS += dlls
}

unix {
  # Make symbol export for shared libs compatible with MSVC
  !CONFIG(staticlib) {
    linux*:QMAKE_CXXFLAGS += -fvisibility-ms-compat
    # *clang*:QMAKE_CXXFLAGS += -fvisibility=hidden
    # The CLang -fvisibility=hidden causes linkage problems at the moment. To be used some later time
  }
}

macx {
  # Dynamic lookup is the best so that circular references do not matter so much
  LIBS += -undefined dynamic_lookup

  # Define @rpath install_name for libraries
  QMAKE_LFLAGS += -Wl,-install_name,@rpath/$$join(TARGET, "", "lib", ".dylib")
}

