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

INSTALLS += target includes src_code extra_inc

# On Windows, put DLLs into /bin with the exes
win32 {
	DLLDESTDIR = $$PWD/bin

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

macx {
    CONFIG += lib_bundle

    TAKE_HEADERS = $$EXPORT_HEADERS
    
    isEmpty(EXPORT_HEADERS) {
      message(Taking all headers)
      TAKE_HEADERS = $$HEADERS
    }
    else {
      message(Selected headers $$EXPORT_HEADERS)
    }

    TAKE_HEADERS += $$EXTRA_HEADERS

    message(More headers $$EXTRA_HEADERS)

    FRAMEWORK_HEADERS.version = Versions
    FRAMEWORK_HEADERS.files = $$TAKE_HEADERS
    FRAMEWORK_HEADERS.path = Headers

    QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS

    isEmpty(EXPORT_SOURCES) {

    }
    else {
      FRAMEWORK_SOURCES.version = Versions
      FRAMEWORK_SOURCES.files = $$EXPORT_SOURCES $$EXTRA_SOURCES
      FRAMEWORK_SOURCES.path = Source

      QMAKE_BUNDLE_DATA += FRAMEWORK_SOURCES

      message(Selected sources $$FRAMEWORK_SOURCES.files)

    }

    message(Creating OSX bundle)
}
