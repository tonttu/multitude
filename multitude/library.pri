include(qmake_utils.prf)

# This is required for installing source code
TARGET_WITHOUT_VERSION=$$TARGET

# Common rules to build libraries
TEMPLATE = lib

# This will disable generation of .so version symlinks on Linux
unix:CONFIG += plugin

# Must strip path from _PRO_FILE since it would duplicate the absolute path
# when installing the source code target
PROJECT_FILE = $$basename($$_PRO_FILE_)

# Make sure we don't override this if it has been set already
isEmpty(DESTDIR):DESTDIR = $$shadowed($$PWD)/lib

isEmpty(EXPORT_HEADERS):EXPORT_HEADERS = $$HEADERS $$EXTRA_HEADERS
isEmpty(EXPORT_SOURCES):EXPORT_SOURCES = $$SOURCES $$EXTRA_SOURCES
contains(EXPORT_SOURCES, nothing) {
  EXPORT_SOURCES=
  PROJECT_FILE=
}

# Default location to install libraries (override in platform specific sections
# below)
target.path = /lib

# Combine all source code into one variable for easier access outside this file
ALL_SOURCE_CODE = $$EXPORT_SOURCES $$EXPORT_HEADERS $$LEXSOURCES $$YACCSOURCES
ALL_SOURCE_CODE += $$PROJECT_FILE

INSTALLS += target

CONFIG(release, debug|release) {
  isEmpty(skip_multitude_install_targets) {
    $$installFiles(/include/$$TARGET_WITHOUT_VERSION, EXPORT_HEADERS)
    $$installFiles(/src/multitude/$$TARGET_WITHOUT_VERSION, ALL_SOURCE_CODE)
  }
}

!no_version_in_target:TARGET=$$join(TARGET,,,$${CORNERSTONE_LIB_SUFFIX})

# On Windows, put DLLs into /bin with the exes
win32 {
  # Optimized debug libraries
  CONFIG(debug,debug|release) {
    CONFIG(optimized) {
      # Set optimization level
      QMAKE_CFLAGS_DEBUG += -O2
      QMAKE_CXXFLAGS_DEBUG += -O2
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
