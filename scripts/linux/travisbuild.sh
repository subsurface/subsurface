
#!/bin/bash

# this gets executed by Travis when building an AppImage for Linux
# it gets started from inside the subsurface directory

export QT_ROOT=$PWD/Qt
export PATH=$QT_ROOT/5.9.1/bin:$PATH # Make sure correct qmake is found on the $PATH for linuxdeployqt
export CMAKE_PREFIX_PATH=$QT_ROOT/5.9.1/lib/cmake 

# the global build script expects to be called from the directory ABOVE subsurface
cd .. 
bash -e ./subsurface/scripts/build.sh -desktop -create-appdir -build-with-webkit # we need to build 'both' and need to build without BT and other variations that we want to exercise

export QT_PLUGIN_PATH=$QT_ROOT/5.9.1/plugins
export QT_QPA_PLATFORM_PLUGIN_PATH=$QT_ROOT/5.9.1/plugins
export QT_DEBUG_PLUGINS=1

# for debugging: find $QT_ROOT/5.9.1/plugins

env CTEST_OUTPUT_ON_FAILURE=1 make -C subsurface/build check

# set up the appdir
mkdir -p appdir/usr/plugins/
mv appdir/usr/home/travis/build/*/subsurface/Qt/5.9.1/plugins/* appdir/usr/plugins/
mv appdir/usr/lib/grantlee/ appdir/usr/plugins/
sudo mv appdir/usr/lib/* /usr/local/lib/ # Workaround for https://github.com/probonopd/linuxdeployqt/issues/160
rm -rf appdir/usr/home/ appdir/usr/include/ appdir/usr/share/man/ # No need to ship developer and man files as part of the AppImage

# get the linuxdeployqt tool and run it to collect the libraries
wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt*.AppImage
unset QTDIR
unset QT_PLUGIN_PATH
unset LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib/ # Workaround for https://github.com/probonopd/linuxdeployqt/issues/160
./linuxdeployqt*.AppImage ./appdir/usr/share/applications/*.desktop -bundle-non-qt-libs -qmldir=./subsurface/map-widget/ -verbose=2

# hack around the gstreamer plugins - this should be fixed with better Qt package
sed -i -e 's|/usr/lib/x86_64-linux-gnu|/usr/lib/x86_64-linux-xxx|g' ./appdir/usr/lib/libgstreamer-1.0.so.0

# create the AppImage
export VERSION=$(cd subsurface/ ; git rev-parse --short HEAD) # linuxdeployqt uses this for naming the file
./linuxdeployqt*.AppImage ./appdir/usr/share/applications/*.desktop -appimage -qmldir=./subsurface/map-widget/ -verbose=2
find ./appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq

# this shouldn't be in the build, but hey, this ensures that we can always
# get to the artifact
curl --upload-file ./Subsurface*.AppImage https://transfer.sh/Subsurface-$VERSION-x86_64.AppImage

