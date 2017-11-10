#!/bin/bash

set -x

# this gets executed by Travis when building an AppImage for Linux
# it gets started from inside the subsurface directory

export PATH=$QT_ROOT/bin:$PATH # Make sure correct qmake is found on the $PATH for linuxdeployqt
export CMAKE_PREFIX_PATH=$QT_ROOT/lib/cmake

# the global build script expects to be called from the directory ABOVE subsurface
cd ..
bash -e -x ./subsurface/scripts/build.sh -desktop -create-appdir -build-with-webkit # we need to build 'both' and need to build without BT and other variations that we want to exercise

export QT_PLUGIN_PATH=$QT_ROOT/plugins
export QT_QPA_PLATFORM_PLUGIN_PATH=$QT_ROOT/plugins
export QT_DEBUG_PLUGINS=1

# for debugging: find $QT_ROOT/plugins

env CTEST_OUTPUT_ON_FAILURE=1 make -C subsurface/build check

# set up the appdir
mkdir -p appdir/usr/plugins/

# mv googlemaps and Grantlee plugins into place
mv appdir/usr/usr/local/Qt*/plugins/* appdir/usr/plugins # the usr/usr is not a typo - that's where it ends up
mv appdir/usr/lib/grantlee/ appdir/usr/plugins/
rm -rf appdir/usr/home/ appdir/usr/include/ appdir/usr/share/man/ # No need to ship developer and man files as part of the AppImage

# get the linuxdeployqt tool and run it to collect the libraries
wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt*.AppImage
unset QTDIR
unset QT_PLUGIN_PATH
unset LD_LIBRARY_PATH
./linuxdeployqt*.AppImage ./appdir/usr/share/applications/*.desktop -bundle-non-qt-libs -qmldir=./subsurface/map-widget/ -verbose=2

# create the AppImage
export VERSION=$(cd subsurface/ ; git rev-parse --short HEAD) # linuxdeployqt uses this for naming the file
./linuxdeployqt*.AppImage ./appdir/usr/share/applications/*.desktop -appimage -qmldir=./subsurface/map-widget/ -verbose=2
find ./appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq

# this shouldn't be in the build, but hey, this ensures that we can always
# get to the artifact
curl --upload-file ./Subsurface*.AppImage https://transfer.sh/Subsurface-$VERSION-x86_64.AppImage

