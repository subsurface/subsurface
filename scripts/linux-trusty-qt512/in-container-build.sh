#!/bin/bash

set -x
set -e

# this gets executed by Travis when building an AppImage for Linux
# inside of the trusty-qt512 container

export PATH=$QT_ROOT/bin:$PATH # Make sure correct qmake is found on the $PATH for linuxdeployqt
export CMAKE_PREFIX_PATH=$QT_ROOT/lib/cmake

# first make sure that no one broke Subsurface-mobile
bash -e -x /subsurface/scripts/build.sh -mobile -quick

# now build our AppImage
bash -e -x /subsurface/scripts/build.sh -desktop -create-appdir -build-with-webkit -quick

export QT_PLUGIN_PATH=$QT_ROOT/plugins
export QT_QPA_PLATFORM_PLUGIN_PATH=$QT_ROOT/plugins
export QT_DEBUG_PLUGINS=1

# set up the appdir
mkdir -p appdir/usr/plugins/

# mv googlemaps plugins into place
mv appdir/usr/usr/local/Qt/5.12.4/gcc_64/plugins/* appdir/usr/plugins  # the usr/usr is not a typo, that's where it ends up
rm -rf appdir/usr/home/ appdir/usr/include/ appdir/usr/share/man/ # No need to ship developer and man files as part of the AppImage
rm -rf appdir/usr/usr appdir/usr/lib/cmake appdir/usr/lib/pkgconfig
cp /ssllibs/libssl.so appdir/usr/lib/libssl.so.1.1
cp /ssllibs/libcrypto.so appdir/usr/lib/libcrypto.so.1.1

# get the linuxdeployqt tool and run it to collect the libraries
curl -L -O "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt*.AppImage
unset QTDIR
unset QT_PLUGIN_PATH
unset LD_LIBRARY_PATH
./linuxdeployqt*.AppImage --appimage-extract-and-run ./appdir/usr/share/applications/*.desktop -bundle-non-qt-libs -qmldir=./subsurface/map-widget/ -verbose=2

# create the AppImage
export VERSION=$(cd /subsurface/scripts ; ./get-version linux) # linuxdeployqt uses this for naming the file
./linuxdeployqt*.AppImage --appimage-extract-and-run ./appdir/usr/share/applications/*.desktop -appimage -qmldir=./subsurface/map-widget/ -verbose=2

# copy AppImage to the calling VM
cp Subsurface*.AppImage* /subsurface
