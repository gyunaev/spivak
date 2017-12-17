#!/bin/bash

# Exit on error
set -e

FILE_VERSION="spivak/src/version.h"

# Checkout the player if we don't have it here
if [ ! -f "spivak.pro" ]; then
    [ -d spivak ] && rm -rf spivak
    git clone --depth 1 https://github.com/gyunaev/spivak.git
    cd spivak
fi

source /opt/qt55/bin/qt55-env.sh || true

qmake -r "CONGIF+=release"
make
cd ..

# Get current version
VERSION_MAJOR=`sed -n 's/^\#define\s\+APP_VERSION_MAJOR\s\+\([0-9]\+\)/\1/p' $FILE_VERSION`
VERSION_MINOR=`sed -n 's/^\#define\s\+APP_VERSION_MINOR\s\+\([0-9]\+\)/\1/p' $FILE_VERSION`
CURRENTVER="$VERSION_MAJOR.$VERSION_MINOR"

# Create the AppImage
APPDIR="spivak.AppDir"

[ -d $APPDIR ] && rm -rf "$APPDIR"
mkdir "$APPDIR"
cd "$APPDIR"

mkdir -p usr/lib64/spivak/plugins usr/bin usr/lib

# Copy the Spivak executable and plugins there
strip --strip-all "../spivak/src/spivak" -o "usr/bin/spivak"
strip --strip-all "../spivak/plugins/languagedetector/plugin_langdetect.so" -o "usr/lib64/spivak/plugins/plugin_langdetect.so"

# Copy the application icon and desktop file into current dir (appimage expects them here)
cp ../spivak/packaging/spivak.desktop .
cp ../spivak/packaging/icon.png spivak.png

# Copy prebuilt libcld2
cp -a /usr/lib64/libcld2.so* usr/lib/

# Unset this as it breaks linuxdeployqt
unset LD_LIBRARY_PATH


#
# Currently we only build AppImage without GStreamer plugins
#

# Copy the other libraries
~/tools/linuxdeployqt-continuous-x86_64.AppImage ../spivak.AppDir/spivak.desktop -bundle-non-qt-libs

# Remove unused files
rm -rf usr/share/doc
rm usr/lib/libgmodule-2.0.so.0
rm usr/lib/libgstapp-1.0.so.0
rm usr/lib/libgstbase-1.0.so.0
rm usr/lib/libgstreamer-1.0.so.0

# Create appimage
(~/tools/appimagetool-x86_64.AppImage . ../Spivak-Host-GStreamer-x86_64.AppImage)
