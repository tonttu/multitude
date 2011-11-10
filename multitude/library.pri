# Common rules to build libraries
TEMPLATE = lib

PROJECT_FILE = $$join(TARGET, "", "", ".pro")

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
src_code.path = /src/MultiTouch/multitude/$$TARGET
src_code.files = $$EXPORT_SOURCES $$EXPORT_HEADERS
src_code.files += $$FLEXSOURCES $$BISONSOURCES
src_code.files += $$PROJECT_FILE

INSTALLS += target

# Source code & headers go with the framework on OS X
INSTALLS += includes src_code extra_inc

# On Windows, put DLLs into /bin with the exes
win32 {
    DLLDESTDIR = $$PWD/bin

	# Debug libraries have an extra extension
  build_pass:CONFIG(debug,debug|release) {
    TARGET=$$join(TARGET,,,_d)
  }

  # Optimized debug libraries
  build_pass:CONFIG(debug,debug|release) {
    CONFIG(optimized) {
      # Set optimization level
	  QMAKE_CFLAGS_DEBUG += -O2
      QMAKE_CXXFLAGS_DEBUG += -O2
      QMAKE_CXXFLAGS_DEBUG=$$replace(QMAKE_CXXFLAGS_DEBUG,-Zi,)
      QMAKE_LFLAGS_DEBUG=$$replace(QMAKE_LFLAGS_DEBUG,/DEBUG,)
      # No need to install headers
      #EXPORT_HEADERS = nothing
    }
  }
	
	# For some reason DESTDIR_TARGET doesn't work here
	tt = $$join(TARGET, "", "$(DESTDIR)", ".dll")
	dlls.path = /bin
	dlls.files += $$tt
	dlls.CONFIG += no_check_exist
	
	INSTALLS += dlls
	
	!isEmpty(WINDOWS_INSTALL_SDK_LIB) {
		# For some reason DESTDIR_TARGET doesn't work here
		sdk_lib.path = /src/MultiTouch/lib
		sdk_lib.files += $$join(TARGET, "", "$(DESTDIR)", ".lib")
		sdk_lib.CONFIG += no_check_exist
	
		sdk_dll.path = /src/MultiTouch/lib
		sdk_dll.files += $$join(TARGET, "", "$(DESTDIR)", ".dll")
		sdk_dll.CONFIG += no_check_exist
		
		INSTALLS += sdk_lib sdk_dll
	}
}

unix {
  # Make symbol export for shared libs compatible with MSVC
  !CONFIG(staticlib) {
    linux*:QMAKE_CXXFLAGS += -fvisibility-ms-compat
  }
}

#ios {
#  message(This is for iOS)
#}

iphone* {
  message(This is for iPhone)
  CONFIG += static
}

macx {
  # Dynamic lookup is the best so that circular references do not matter so much
  LIBS += -undefined dynamic_lookup
  !iphone* {
    CONFIG += lib_bundle

    target.path = /Library/Frameworks

    FRAMEWORK_HEADERS.version = Versions
    FRAMEWORK_HEADERS.files = $$EXPORT_HEADERS $$EXTRA_HEADERS
    FRAMEWORK_HEADERS.path = Headers

    QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS

    !isEmpty(EXPORT_SOURCES) {

      FRAMEWORK_SOURCES.version = Versions
      FRAMEWORK_SOURCES.files = $$EXPORT_SOURCES $$EXTRA_SOURCES
      FRAMEWORK_SOURCES.path = Source

      QMAKE_BUNDLE_DATA += FRAMEWORK_SOURCES
    }
  }
}
