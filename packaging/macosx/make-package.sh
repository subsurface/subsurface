#!/bin/bash
#
# this simply automates the steps to create a DMG we can ship
#
# for this to work you need to have a custom build of MacPorts / gtk / etc
# with prefix=/Applications/Subsurface.app/Contents/Resources
# yes, that's a major hack, but otherwise gettext cannot seem to find
# the gtk related .mo files and localization is only partial
#
# run this from the top subsurface directory

# adjust to your install location of gtk-mac-bundler. I appear to need at
# least 0.7.2
BUNDLER="../.local/bin/gtk-mac-bundler"
BUNDLER_SRC="${HOME}/gtk-mac-bundler"

# install location of yourway-create-dmg
DMGCREATE="../yoursway-create-dmg/create-dmg"

# This is the directory into which MacPorts, libdivecomputer and all the
# other components have been installed
PREFIX="/Applications/Subsurface.app/Contents/Resources"

INFOPLIST=./packaging/macosx/Info.plist

# same git version magic as in the Makefile
VERSION=`git describe --tags --abbrev=12 | sed 's/v\([0-9]*\)\.\([0-9]*\)-\([0-9]*\)-.*/\1.\2.\3/ ; s/v\([0-9]\)\.\([0-9]*\)/\1.\2.0/' || echo "git.missing.please.hardcode.version"`

# gtk-mac-bundler allegedly supports signing by setting this environment
# variable, but this fails as we change the shared objects below and all
# the signatures become invalid.
# export APPLICATION_CERT="Dirk"

# force rebuilding of Info.plist
rm ${INFOPLIST}

# first build and install Subsurface and then clean up the staging area
make
make install-macosx
rm -rf ./staging

# now populate it with the bundle
${BUNDLER} packaging/macosx/subsurface.bundle

# correct the paths and names
cd staging/Subsurface.app/Contents
for i in Resources/lib/gdk-pixbuf-2.0/2.10.0/loaders/* ; do
	${BUNDLER_SRC}/bundler/run-install-name-tool-change.sh $i ${PREFIX} Resources change ;
done
for i in Resources/lib/*.dylib;
do
	install_name_tool -id "@executable_path/../$i" $i
done

cd ../../..

codesign -s Dirk ./staging/Subsurface.app/Contents/MacOS/subsurface \
	./staging/Subsurface.app/Contents/MacOS/subsurface-bin

if [ -f ./Subsurface-${VERSION}.dmg ]; then
	rm ./Subsurface-${VERSION}.dmg.bak
	mv ./Subsurface-${VERSION}.dmg ./Subsurface-${VERSION}.dmg.bak
fi

${DMGCREATE} --background ./packaging/macosx/DMG-Background.png \
	--window-size 500 300 --icon-size 96 --volname Subsurface-${VERSION} \
	--app-drop-link 380 205 \
	--volicon ~/subsurface/packaging/macosx/Subsurface.icns \
	--icon "Subsurface" 110 205 ./Subsurface-${VERSION}.dmg ./staging

