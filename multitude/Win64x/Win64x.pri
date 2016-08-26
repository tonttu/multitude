# Construct path to dependency package
CORNERSTONE_FULL_VERSION_STR = $$cat(../../VERSION)

CORNERSTONE_VERSION = $$section(CORNERSTONE_FULL_VERSION_STR, "-", 0, 0)
CORNERSTONE_VERSION_MAJOR = $$section(CORNERSTONE_VERSION, ".", 0, 0)
CORNERSTONE_VERSION_MINOR = $$section(CORNERSTONE_VERSION, ".", 1, 1)
CORNERSTONE_VERSION_PATCH = $$section(CORNERSTONE_VERSION, ".", 2, 2)
CORNERSTONE_SHORT_VERSION_STR = $${CORNERSTONE_VERSION_MAJOR}.$${CORNERSTONE_VERSION_MINOR}

CORNERSTONE_DEPS_PATH=C:/Cornerstone-$${CORNERSTONE_SHORT_VERSION_STR}-deps

# Install Windows 3rd party dlls to bin
win64_runtime_dlls.path = /bin
win64_runtime_dlls.files = $$PWD/bin64/*
win64_runtime_dlls.files += $$CORNERSTONE_DEPS_PATH/ssl/*dll

# Install libraries under 'src'
win64_sdk_libs1.path = /src/multitude/Win64x/lib64
win64_sdk_libs1.files = $$PWD/lib64/*

# Install libraries under 'lib'
win64_sdk_libs2.path = /lib
win64_sdk_libs2.files = $$PWD/lib64/*

# Install dlls under 'src'
win64_sdk_dlls.path = /src/multitude/Win64x/bin64
win64_sdk_dlls.files = $$PWD/bin64/*

# Install headers under 'src'
win64_sdk_headers1.path = /src/multitude/Win64x/include
win64_sdk_headers1.files = $$PWD/include/*

# Install headers under 'include'
win64_sdk_headers2.path = /include
win64_sdk_headers2.files = $$PWD/include/*

# Install project files under 'src'
win64_sdk_project.path = /src/multitude/Win64x
win64_sdk_project.files = $$PWD/Win64x.pri


win64_libav_bins.path = /bin
win64_libav_bins2.path = /src/multitude/Win64x/bin64

win64_libav_libs1.path = /lib
win64_libav_libs2.path = /src/multitude/Win64x/lib64

win64_libav_headers1.path = /include
win64_libav_headers2.path = /src/multitude/Win64x/include

TEST=$$(USE_LIBAV)
isEmpty(TEST) {

  win64_libav_bins.files = $$CORNERSTONE_DEPS_PATH/ffmpeg/bin/*dll
  win64_libav_bins.files += $$CORNERSTONE_DEPS_PATH/ffmpeg/bin/*exe

  win64_libav_libs1.files = $$CORNERSTONE_DEPS_PATH/ffmpeg/bin/*lib
  win64_libav_libs2.files = $$CORNERSTONE_DEPS_PATH/ffmpeg/bin/*lib

  win64_libav_headers1.files = $$CORNERSTONE_DEPS_PATH/ffmpeg/include/*
  win64_libav_headers2.files = $$CORNERSTONE_DEPS_PATH/ffmpeg/include/*

} else {

  win64_libav_bins.files = $$CORNERSTONE_DEPS_PATH/libav/bin/*dll
  win64_libav_bins.files += $$CORNERSTONE_DEPS_PATH/libav/bin/*exe

  win64_libav_libs1.files = $$CORNERSTONE_DEPS_PATH/libav/bin/*lib
  win64_libav_libs2.files = $$CORNERSTONE_DEPS_PATH/libav/bin/*lib

  win64_libav_headers1.files = $$CORNERSTONE_DEPS_PATH/libav/include/*
  win64_libav_headers2.files = $$CORNERSTONE_DEPS_PATH/libav/include/*

}

win64_libav_bins2.files = $${win64_libav_bins.files}


INSTALLS += win64_libav_bins
INSTALLS += win64_libav_bins2
INSTALLS += win64_libav_libs1
INSTALLS += win64_libav_libs2
INSTALLS += win64_libav_headers1
INSTALLS += win64_libav_headers2

win64_ghostscript.path = /bin
win64_ghostscript.files = $$CORNERSTONE_DEPS_PATH/ghostscript/*

!enable-taction:INSTALLS += win64_ghostscript

win64_node_dlls1.path = /bin
win64_node_dlls1.files = $$CORNERSTONE_DEPS_PATH/node/bin/node.exe
win64_node_dlls1.files += $$CORNERSTONE_DEPS_PATH/node/bin/*.cmd
win64_node_dlls1.files += $$CORNERSTONE_DEPS_PATH/node/bin/*.dll
win64_node_dlls1.files += $$CORNERSTONE_DEPS_PATH/node/bin/node_modules

win64_node_dlls2.path = /src/multitude/Win64x/bin64
win64_node_dlls2.files = $$CORNERSTONE_DEPS_PATH/node/bin/node.exe
win64_node_dlls2.files += $$CORNERSTONE_DEPS_PATH/node/bin/*.cmd
win64_node_dlls2.files += $$CORNERSTONE_DEPS_PATH/node/bin/*.dll
win64_node_dlls2.files += $$CORNERSTONE_DEPS_PATH/node/bin/node_modules

win64_node_libs1.path = /lib
win64_node_libs1.files = $$CORNERSTONE_DEPS_PATH/node/lib/*lib

win64_node_libs2.path = /src/multitude/Win64x/lib64
win64_node_libs2.files = $$CORNERSTONE_DEPS_PATH/node/lib/*lib

win64_node_headers1.path = /include
win64_node_headers1.files = $$CORNERSTONE_DEPS_PATH/node/include/*

win64_node_headers2.path = /src/multitude/Win64x/include
win64_node_headers2.files = $$CORNERSTONE_DEPS_PATH/node/include/*

win64_argyll.path = /bin
win64_argyll.files = $$CORNERSTONE_DEPS_PATH/argyll/spotread.exe

win64_ruby.path = /tools
win64_ruby.files = $$CORNERSTONE_DEPS_PATH/ruby

win64_clang_headers.path = /include/libcxx
win64_clang_headers.files = $$CORNERSTONE_DEPS_PATH/libcxx/include/*

win64_clangxml.path = /bin
win64_clangxml.files = $$CORNERSTONE_DEPS_PATH/clangxml/ClangXML.exe

win64_boost_headers1.path = /include
win64_boost_headers1.files = $$CORNERSTONE_DEPS_PATH/boost_1_55_0/boost

win64_boost_headers2.path = /src/multitude/Win64x/include
win64_boost_headers2.files = $${win64_boost_headers1.files}

!enable-taction {
  win64_curl_dlls1.path = /bin
  win64_curl_dlls1.files = $$CORNERSTONE_DEPS_PATH/curl/dlls/*

  win64_curl_dlls2.path = /src/multitude/Win64x/bin64
  win64_curl_dlls2.files = $${win64_curl_dlls1.files}

  win64_curl_libs1.path = /lib
  win64_curl_libs1.files = $$CORNERSTONE_DEPS_PATH/curl/lib/*

  win64_curl_libs2.path = /src/multitude/Win64x/lib64
  win64_curl_libs2.files = $${win64_curl_libs1.files}

  win64_curl_headers1.path = /include
  win64_curl_headers1.files = $$CORNERSTONE_DEPS_PATH/curl/include/*

  win64_curl_headers2.path = /src/multitude/Win64x/include
  win64_curl_headers2.files = $${win64_curl_headers1.files}

  INSTALLS += win64_node_dlls1
  INSTALLS += win64_node_dlls2
  INSTALLS += win64_node_libs1
  INSTALLS += win64_node_libs2
  INSTALLS += win64_node_headers1
  INSTALLS += win64_node_headers2
  INSTALLS += win64_ruby
  INSTALLS += win64_clang_headers
  INSTALLS += win64_clangxml
}
INSTALLS += win64_argyll
INSTALLS += win64_runtime_dlls
INSTALLS += win64_sdk_libs1
INSTALLS += win64_sdk_libs2
INSTALLS += win64_sdk_dlls
INSTALLS += win64_sdk_headers1
INSTALLS += win64_sdk_headers2
INSTALLS += win64_sdk_project
INSTALLS += win64_curl_dlls1
INSTALLS += win64_curl_dlls2
INSTALLS += win64_curl_libs1
INSTALLS += win64_curl_libs2
INSTALLS += win64_curl_headers1
INSTALLS += win64_curl_headers2
INSTALLS += win64_boost_headers1
INSTALLS += win64_boost_headers2

# Install Qt
enable-taction {
  qt_bin_files.path = /bin
  qt_bin_files.files = $$files($$[QT_INSTALL_BINS]/*.dll)
  qt_bin_files.files -= $$files($$[QT_INSTALL_BINS]/*d4.dll)
  qt_bin_files.files -= $$files($$[QT_INSTALL_BINS]/*WebKit*.dll)
} else {
  qt_bin_files.path = /
  qt_bin_files.files = $$[QT_INSTALL_BINS]
}
qt_conf_files.path = /bin
qt_conf_files.files = $$PWD/qt.conf

qt_lib_files.path = /qt/lib
qt_lib_files.files = $$[QT_INSTALL_LIBS]\\*.lib

qt_files.path = /qt
qt_files.files += $$[QT_INSTALL_PLUGINS]
qt_files.files += $$[QT_INSTALL_IMPORTS]
qt_files.files += $$[QT_INSTALL_TRANSLATIONS]
qt_files.files += $$[QMAKE_MKSPECS]
qt_files.files += $$[QT_INSTALL_HEADERS]

INSTALLS += qt_bin_files qt_lib_files qt_files qt_conf_files

# Install Cef
win64_cef_dlls.path = /bin
win64_cef_dlls.files = $$CORNERSTONE_DEPS_PATH/cef/Release/*

win64_cef_resources.path = /Resources
win64_cef_resources.files = $$CORNERSTONE_DEPS_PATH/cef/Resources/*

INSTALLS += win64_cef_dlls win64_cef_resources

message(Including 64-bit Windows Libraries)
