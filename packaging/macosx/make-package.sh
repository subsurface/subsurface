#!/bin/sh
#
# this simply automates the steps to create a DMG we can ship
#
# run this from the top subsurface directory

# find the directory above the sources - typically ~/src
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd ../../.. && pwd )

# install location of yourway-create-dmg
# by default we assume it's next to subsurface in ~/src/yoursway-create-dmg
DMGCREATE=${DIR}/yoursway-create-dmg/create-dmg

# same git version magic as in the Makefile
# for the naming of volume and dmg we want the 3 digits of the full version number
VERSION=$(cd ${DIR}/subsurface; ./scripts/get-version linux)

# first build and install Subsurface and then clean up the staging area
rm -rf ./Subsurface.app
LIBRARY_PATH=${DIR}/install-root/lib make -j8
LIBRARY_PATH=${DIR}/install-root/lib make install

# HACK TIME... QtXml is missing. screw this
cp -a $HOME/Qt/5.5/clang_64/lib/QtXml.framework Subsurface.app/Contents/Frameworks
rm -rf Subsurface.app/Contents/Frameworks/QtXml.framework/Versions/5/Headers
rm -rf Subsurface.app/Contents/Frameworks/QtXml.framework/Headers
rm -rf Subsurface.app/Contents/Frameworks/QtXml.framework/QtXml.prl
rm -rf Subsurface.app/Contents/Frameworks/QtXml.framework/Versions/5/*_debug
rm -rf Subsurface.app/Contents/Frameworks/QtXml.framework/*_debug*
install_name_tool -id @executable_path/../Frameworks/QtXml Subsurface.app/Contents/Frameworks/QtXml.framework/QtXml
install_name_tool -change @rpath/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/QtCore Subsurface.app/Contents/Frameworks/QtXml.framework/QtXml

# now adjust a few references that macdeployqt appears to miss
EXECUTABLE=Subsurface.app/Contents/MacOS/Subsurface
for i in libssh libssrfmarblewidget libgit2; do
	OLD=$(otool -L ${EXECUTABLE} | grep $i | cut -d\  -f1 | tr -d "\t")
	cp ${DIR}/install-root/lib/$(basename ${OLD}) Subsurface.app/Contents/Frameworks
	SONAME=$(basename $OLD)
	install_name_tool -change ${OLD} @executable_path/../Frameworks/${SONAME} ${EXECUTABLE}
	if [[ "$i" = "libssh" ]] ; then
		LIBSSH=$(basename ${OLD})
	fi
	if [[ "$i" = "libgit2" ]] ; then
		install_name_tool -change ${LIBSSH} @executable_path/../Frameworks/${LIBSSH} Subsurface.app/Contents/Frameworks/${SONAME}
	fi
done
RPATH=$(otool -L ${EXECUTABLE} | grep rpath  | cut -d\  -f1 | tr -d "\t" | cut -b 8- )
for i in ${RPATH}; do
	install_name_tool -change @rpath/$i @executable_path/../Frameworks/$i ${EXECUTABLE}
done

# next deal with libGrantlee
LIBG=Subsurface.app/Contents/Frameworks/libGrantlee_Templates.5.dylib
for i in QtScript.framework/Versions/5/QtScript QtCore.framework/Versions/5/QtCore ; do
	install_name_tool -change @rpath/$i @executable_path/../Frameworks/$i ${LIBG}
done

# it seems the compiler in XCode 4.6 doesn't build Grantlee5 correctly,
# so cheat and copy over pre-compiled binaries created with a newer compiler
# and adjust their references to the Grantlee template library
#
# -disabled for now as this is still under more investigation-
# cp -a /Users/hohndel/src/tmp/Subsurface.app/Contents Subsurface.app/
cp ${DIR}/tmp/Subsurface.app/Contents/Frameworks/lib{sql,usb,zip}* Subsurface.app/Contents/Frameworks

# clean up shared library dependency in the Grantlee plugins
for i in Subsurface.app/Contents/PlugIns/grantlee/5.0/*.so; do
	OLD=$(otool -L $i | grep libGrantlee_Templates | cut -d\  -f1 | tr -d "\t")
	SONAME=$(basename $OLD )
	install_name_tool -change ${OLD} @executable_path/../Frameworks/${SONAME} $i;
	mv $i Subsurface.app/Contents/PlugIns/grantlee
done
rmdir Subsurface.app/Contents/PlugIns/grantlee/5.0

# copy things into staging so we can create a nice DMG
rm -rf ./staging
mkdir ./staging
cp -a ./Subsurface.app ./staging

sh ${DIR}/subsurface/packaging/macosx/sign

if [ -f ./Subsurface-$VERSION.dmg ]; then
	rm ./Subsurface-$VERSION.dmg.bak
	mv ./Subsurface-$VERSION.dmg ./Subsurface-$VERSION.dmg.bak
fi

$DMGCREATE --background ${DIR}/subsurface/packaging/macosx/DMG-Background.png \
	--window-size 500 300 --icon-size 96 --volname Subsurface-$VERSION \
	--app-drop-link 380 205 \
	--volicon ${DIR}/subsurface/packaging/macosx/Subsurface.icns \
	--icon "Subsurface" 110 205 ./Subsurface-$VERSION.dmg ./staging
