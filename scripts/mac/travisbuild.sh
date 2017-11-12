#!/bin/bash

# this gets executed by Travis when building an App for Mac
# it gets started from inside the subsurface directory

export QT_ROOT=${TRAVIS_BUILD_DIR}/Qt/5.9.1
export PATH=$QT_ROOT/bin:$PATH # Make sure correct qmake is found on the $PATH
export CMAKE_PREFIX_PATH=$QT_ROOT/lib/cmake
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

# the global build script expects to be called from the directory ABOVE subsurface
cd ${TRAVIS_BUILD_DIR}/..
DIR=$(pwd)

#bash -e -x ./subsurface/scripts/build.sh -desktop -build-with-webkit # we need to build 'both' and need to build without BT and other variations that we want to exercise
bash -e -x ./subsurface/scripts/build.sh -desktop -build-with-webkit -skip-googlemaps

cd ${TRAVIS_BUILD_DIR}/build

# first build and install Subsurface and then clean up the staging area
LIBRARY_PATH=${DIR}/install-root/lib make -j2 install

# now adjust a few references that macdeployqt appears to miss
EXECUTABLE=Subsurface.app/Contents/MacOS/Subsurface
for i in libgit2 libGrantlee_TextDocument.dylib libGrantlee_Templates.dylib; do
	OLD=$(otool -L ${EXECUTABLE} | grep $i | cut -d\  -f1 | tr -d "\t")
	if [ ! -z ${OLD} ] ; then
		# copy the library into the bundle and make sure its id and the reference to it are correct
		cp ${DIR}/install-root/lib/$(basename ${OLD}) Subsurface.app/Contents/Frameworks
		SONAME=$(basename $OLD)
		install_name_tool -change ${OLD} @executable_path/../Frameworks/${SONAME} ${EXECUTABLE}
		install_name_tool -id @executable_path/../Frameworks/${SONAME} Subsurface.app/Contents/Frameworks/${SONAME}
		# also fix incorrect references inside of libgit2
		if [[ "$i" = "libgit2" ]] ; then
			CURLLIB=$(otool -L Subsurface.app/Contents/Frameworks/${SONAME} | grep libcurl | cut -d\  -f1 | tr -d "\t")
			install_name_tool -change ${CURLLIB} @executable_path/../Frameworks/$(basename ${CURLLIB}) Subsurface.app/Contents/Frameworks/${SONAME}
			SSHLIB=$(otool -L Subsurface.app/Contents/Frameworks/${SONAME} | grep libssh2 | cut -d\  -f1 | tr -d "\t")
			install_name_tool -change ${SSHLIB} @executable_path/../Frameworks/$(basename ${SSHLIB}) Subsurface.app/Contents/Frameworks/${SONAME}
		fi
	fi
done

# next, copy libssh2.1
cp ${DIR}/install-root/lib/libssh2.1.dylib Subsurface.app/Contents/Frameworks

# next, replace @rpath references with @executable_path references in Subsurface
RPATH=$(otool -L ${EXECUTABLE} | grep rpath  | cut -d\  -f1 | tr -d "\t" | cut -b 8- )
for i in ${RPATH}; do
	install_name_tool -change @rpath/$i @executable_path/../Frameworks/$i ${EXECUTABLE}
done

# next deal with libGrantlee
LIBG=$(ls Subsurface.app/Contents/Frameworks/libGrantlee_Templates*dylib)
for i in QtScript.framework/Versions/5/QtScript QtCore.framework/Versions/5/QtCore ; do
	install_name_tool -change @rpath/$i @executable_path/../Frameworks/$i ${LIBG}
done

# clean up shared library dependency in the Grantlee plugins
for i in Subsurface.app/Contents/PlugIns/grantlee/5.0/*.so; do
	OLD=$(otool -L $i | grep libGrantlee_Templates | cut -d\  -f1 | tr -d "\t")
	SONAME=$(basename $OLD )
	install_name_tool -change ${OLD} @executable_path/../Frameworks/${SONAME} $i;
	OLD=$(otool -L $i | grep QtCore | cut -d\  -f1 | tr -d "\t")
	install_name_tool -change ${OLD} @executable_path/../Frameworks/QtCore.framework/QtCore $i;
	mv $i Subsurface.app/Contents/PlugIns/grantlee
done
rmdir Subsurface.app/Contents/PlugIns/grantlee/5.0
pushd Subsurface.app/Contents/PlugIns/grantlee
ln -s . 5.0
popd
