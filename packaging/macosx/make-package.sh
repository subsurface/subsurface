#!/bin/sh
#
# this simply automates the steps to create a DMG we can ship
#
# for this to work you need to have a custom build of MacPorts / gtk / etc
# with prefix=/Applications/Subsurface.app/Contents/Resources
# yes, that's a major hack, but otherwise gettext cannot seem to find
# the gtk related .mo files and localization is only partial
#
# run this from the top subsurface directory

# install location of yourway-create-dmg
DMGCREATE=../yoursway-create-dmg/create-dmg

# same git version magic as in the Makefile
# for the naming of volume and dmg we don't need the "always 3 digits"
# darwin version - the 'regular' version that has 2 digits for releases
# is better
VERSION=$(cd ../subsurface; ./scripts/get-version linux)


# first build and install Subsurface and then clean up the staging area
rm -rf ./Subsurface.app
make -j8
make install
install_name_tool -change /Users/hohndel/src/marble/install/libssrfmarblewidget.0.19.2.dylib @executable_path/../Frameworks/libssrfmarblewidget.0.19.2.dylib Subsurface.app/Contents/MacOS/Subsurface
install_name_tool -change /Users/hohndel/src/libgit2/build/libgit2.22.dylib @executable_path/../Frameworks/libgit2.22.dylib Subsurface.app/Contents/MacOS/Subsurface

# copy things into staging so we can create a nice DMG
rm -rf ./staging
mkdir ./staging
cp -a ./Subsurface.app ./staging

sh ../subsurface/packaging/macosx/sign

if [ -f ./Subsurface-$VERSION.dmg ]; then
	rm ./Subsurface-$VERSION.dmg.bak
	mv ./Subsurface-$VERSION.dmg ./Subsurface-$VERSION.dmg.bak
fi

$DMGCREATE --background ../subsurface/packaging/macosx/DMG-Background.png \
	--window-size 500 300 --icon-size 96 --volname Subsurface-$VERSION \
	--app-drop-link 380 205 \
	--volicon ../subsurface/packaging/macosx/Subsurface.icns \
	--icon "Subsurface" 110 205 ./Subsurface-$VERSION.dmg ./staging
