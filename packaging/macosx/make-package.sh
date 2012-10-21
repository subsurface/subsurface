#!/bin/bash
#
# this simply automates the steps to create a DMG we can ship
# for this to work you need to have a custom build of MacPorts / gtk / etc
# with prefix=/Applications/Subsurface.app/Contents/Resources
# yes, that's a major hack, but otherwise gettext cannot seem to find
# the gtk related .mo files and localization is only partial
#
# run this from the packaging/macosx directory

VERSION=`grep -1 CFBundleVersionString Info.plist | tail -1 | cut -d\> -f 2 | cut -d\< -f 1`
BUNDLER="../../../.local/bin/gtk-mac-bundler"

${BUNDLER} subsurface.bundle
cd staging/Subsurface.app/Contents
for i in Resources/lib/gdk-pixbuf-2.0/2.10.0/loaders/* ; do
	~/gtk-mac-bundler/bundler/run-install-name-tool-change.sh $i /Applications/Subsurface.app/Contents/Resources Resources change ;
done
for i in Resources/lib/*.dylib;
do
	install_name_tool -id "@executable_path/../$i" $i
done

cd ../../..
if [ -f Subsurface-${VERSION}.dmg ]; then
	mv Subsurface-${VERSION}.dmg Subsurface-${VERSION}.dmg.bak
fi
hdiutil create -volname Subsurface -srcfolder staging Subsurface-${VERSION}.dmg

