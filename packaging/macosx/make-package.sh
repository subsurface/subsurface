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
# make sure we didn't lose the minimum OS version
rm -rf ./Subsurface.app
if [ -d /Developer/SDKs ] ; then
	SDKROOT=/Developer/SDKs
elif [ -d /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs ] ; then
	SDKROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs
else
	echo "Cannot find SDK sysroot (usually /Developer/SDKs or"
	echo "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs)"
	exit 1;
fi
BASESDK=$(ls $SDKROOT | grep "MacOSX10\.1.\.sdk" | head -1 | sed -e "s/MacOSX//;s/\.sdk//")
OLDER_MAC_CMAKE="-DCMAKE_OSX_DEPLOYMENT_TARGET=${BASESDK} -DCMAKE_OSX_SYSROOT=${SDKROOT}/MacOSX${BASESDK}.sdk/"
export PKG_CONFIG_PATH=${DIR}/install-root/lib/pkgconfig:$PKG_CONFIG_PATH
cmake $OLDER_MAC_CMAKE .
LIBRARY_PATH=${DIR}/install-root/lib make -j8
LIBRARY_PATH=${DIR}/install-root/lib make install

# now adjust a few references that macdeployqt appears to miss
EXECUTABLE=Subsurface.app/Contents/MacOS/Subsurface
for i in libgit2 ; do
	OLD=$(otool -L ${EXECUTABLE} | grep $i | cut -d\  -f1 | tr -d "\t")
	if [[ ! -z ${OLD} && ! -f Subsurface.app/Contents/Frameworks/$(basename ${OLD}) ]] ; then
		# copy the library into the bundle and make sure its id and the reference to it are correct
		cp ${DIR}/install-root/lib/$(basename ${OLD}) Subsurface.app/Contents/Frameworks
		SONAME=$(basename $OLD)
		install_name_tool -change ${OLD} @executable_path/../Frameworks/${SONAME} ${EXECUTABLE}
		install_name_tool -id @executable_path/../Frameworks/${SONAME} Subsurface.app/Contents/Frameworks/${SONAME}
		# also fix incorrect references inside of libgit2
		if [[ "$i" = "libgit2" ]] ; then
			CURLLIB=$(otool -L Subsurface.app/Contents/Frameworks/${SONAME} | grep libcurl | cut -d\  -f1 | tr -d "\t")
			install_name_tool -change ${CURLLIB} @executable_path/../Frameworks/$(basename ${CURLLIB}) Subsurface.app/Contents/Frameworks/${SONAME}
			# SSHLIB=$(otool -L Subsurface.app/Contents/Frameworks/${SONAME} | grep libssh2 | cut -d\  -f1 | tr -d "\t")
			# install_name_tool -change ${SSHLIB} @executable_path/../Frameworks/$(basename ${SSHLIB}) Subsurface.app/Contents/Frameworks/${SONAME}
		fi
	fi
done

# ensure libpng and libjpeg inside the bundle are referenced in QtWebKit libraries
QTWEBKIT=Subsurface.app/Contents/Frameworks/QtWebKit.framework/QtWebKit
for i in libjpeg.8.dylib libpng16.16.dylib; do
	OLD=$(otool -L ${QTWEBKIT} | grep $i | cut -d\  -f1 | tr -d "\t")
        if [[ ! -z ${OLD} ]] ; then
                # copy the library into the bundle and make sure its id and the reference to it are correct
		if [[ ! -f Subsurface.app/Contents/Frameworks/$(basename ${OLD}) ]] ; then
			cp ${OLD} Subsurface.app/Contents/Frameworks
		fi
                SONAME=$(basename $OLD)
                install_name_tool -change ${OLD} @executable_path/../Frameworks/${SONAME} ${QTWEBKIT}
                install_name_tool -id @executable_path/../Frameworks/${SONAME} Subsurface.app/Contents/Frameworks/${SONAME}
        fi
done

# next, copy libssh2.1
# cp ${DIR}/install-root/lib/libssh2.1.dylib Subsurface.app/Contents/Frameworks

# next, replace @rpath references with @executable_path references in Subsurface
RPATH=$(otool -L ${EXECUTABLE} | grep rpath  | cut -d\  -f1 | tr -d "\t" | cut -b 8- )
for i in ${RPATH}; do
	install_name_tool -change @rpath/$i @executable_path/../Frameworks/$i ${EXECUTABLE}
done

if [ "$1" = "-nodmg" ] ; then
	exit 0
fi

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
