#!/bin/bash

set -evx

rm -rf pdfium
rm -rf workspace

mkdir workspace
cd workspace

echo "Cloning depot tools"
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git

export PATH=`pwd`/depot_tools:$PATH
gclient config --unmanaged https://pdfium.googlesource.com/pdfium.git

echo "Cloning pdfium"
gclient sync

cd pdfium

export BUILD_DIR=out/build

gn gen $BUILD_DIR

#see list with 'gn args $BUILD_DIR --list'
echo "use_goma = false" > $BUILD_DIR/args.gn
echo "is_debug = false" >> $BUILD_DIR/args.gn
echo "is_official_build = true" >> $BUILD_DIR/args.gn
echo "pdf_use_skia = false" >> $BUILD_DIR/args.gn
echo "pdf_enable_xfa = false" >> $BUILD_DIR/args.gn
echo "pdf_enable_v8 = false" >> $BUILD_DIR/args.gn
echo "pdf_is_standalone = true" >> $BUILD_DIR/args.gn

# For some reason we need gn gen again after fiddling with arguments.
# Would be amazed if there wouldn't be better way to do this, maybe just 
# mkdir, arguments and gn gen...


gn clean $BUILD_DIR
gn gen $BUILD_DIR

echo "Compile pdfium"
ninja -C $BUILD_DIR

cd ../
mv pdfium ../
cd ../
rm -fr workspace
