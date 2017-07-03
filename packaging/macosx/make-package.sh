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

# now adjust a few references that macdeployqt appears to miss
EXECUTABLE=Subsurface.app/Contents/MacOS/Subsurface
for i in libssh libssrfmarblewidget libgit2 libGrantlee_TextDocument.dylib libGrantlee_Templates.dylib; do
	OLD=$(otool -L ${EXECUTABLE} | grep $i | cut -d\  -f1 | tr -d "\t")
	if [ ! -z ${OLD} ] ; then
		# copy the library into the bundle and make sure its id and the reference to it are correct
		cp ${DIR}/install-root/lib/$(basename ${OLD}) Subsurface.app/Contents/Frameworks
		SONAME=$(basename $OLD)
		install_name_tool -change ${OLD} @executable_path/../Frameworks/${SONAME} ${EXECUTABLE}
		install_name_tool -id @executable_path/../Frameworks/${SONAME} Subsurface.app/Contents/Frameworks/${SONAME}
		# also fix one incorrect reference inside of libgit2
		if [[ "$i" = "libgit2" ]] ; then
			CURLLIB=$(otool -L Subsurface.app/Contents/Frameworks/${SONAME} | grep libcurl | cut -d\  -f1 | tr -d "\t")
			install_name_tool -change ${CURLLIB} @executable_path/../Frameworks/$(basename ${CURLLIB}) Subsurface.app/Contents/Frameworks/${SONAME}
		fi
	fi
done

# next, replace @rpath references with @executable_path references in Subsurface
RPATH=$(otool -L ${EXECUTABLE} | grep rpath  | cut -d\  -f1 | tr -d "\t" | cut -b 8- )
for i in ${RPATH}; do
	install_name_tool -change @rpath/$i @executable_path/../Frameworks/$i ${EXECUTABLE}
done

# and now replace @rpath references in libssrfmarblewidget
MARBLELIB=$(ls Subsurface.app/Contents/Frameworks/libssrfmarblewidget*dylib)
RPATH=$(otool -L ${MARBLELIB} | grep rpath  | cut -d\  -f1 | tr -d "\t" | cut -b 8- )
for i in ${RPATH}; do
	install_name_tool -change @rpath/$i @executable_path/../Frameworks/$i ${MARBLELIB}
done

# next deal with libGrantlee
LIBG=$(ls Subsurface.app/Contents/Frameworks/libGrantlee_Templates*dylib)
for i in QtScript.framework/Versions/5/QtScript QtCore.framework/Versions/5/QtCore ; do
	install_name_tool -change @rpath/$i @executable_path/../Frameworks/$i ${LIBG}
done

# it seems the compiler in XCode 4.6 doesn't build Grantlee5 correctly,
# so cheat and copy over pre-compiled binaries created with a newer compiler
# and adjust their references to the Grantlee template library
#
# -disabled for now as this is still under more investigation-
# cp -a /Users/hohndel/src/tmp/Subsurface.app/Contents Subsurface.app/
#cp ${DIR}/tmp/Subsurface.app/Contents/Frameworks/lib{sql,usb,zip}* Subsurface.app/Contents/Frameworks

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
