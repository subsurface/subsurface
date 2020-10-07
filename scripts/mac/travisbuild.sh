#!/bin/bash

set -x
set -e

# this gets executed by Travis when building an App for Mac
# it gets started from inside the subsurface directory

export QT_ROOT=${TRAVIS_BUILD_DIR}/Qt/5.12.3/clang_64
export PATH=$QT_ROOT/bin:$PATH # Make sure correct qmake is found on the $PATH
export CMAKE_PREFIX_PATH=$QT_ROOT/lib/cmake
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

# the global build script expects to be called from the directory ABOVE subsurface
cd ${TRAVIS_BUILD_DIR}/..
DIR=$(pwd)

# first build Subsurface-mobile to ensure this didn't get broken
bash -e -x ./subsurface/scripts/build.sh -mobile

# now Subsurface with WebKit
bash -e -x ./subsurface/scripts/build.sh -desktop -build-with-webkit -release

cd ${TRAVIS_BUILD_DIR}/build

# build export-html to make sure that didn't get broken
make export-html

# first build and install Subsurface and then clean up the staging area
LIBRARY_PATH=${DIR}/install-root/lib make -j2 install

# now adjust a few references that macdeployqt appears to miss
# there used to be more - having the for-loop for just one seems overkill, but I
# wouldn't be surprised if there will be more again in the future, so leave it for now
EXECUTABLE=Subsurface.app/Contents/MacOS/Subsurface
for i in libgit2 ; do
	OLD=$(otool -L ${EXECUTABLE} | grep $i | cut -d\  -f1 | tr -d "\t")
	if [[ ! -z ${OLD} && ! -f Subsurface.app/Contents/Frameworks/$(basename ${OLD}) ]] ; then
		# copy the library into the bundle and make sure its id and the reference to it are correct
		cp ${DIR}/install-root/lib/$(basename ${OLD}) Subsurface.app/Contents/Frameworks
		SONAME=$(basename $OLD)
		install_name_tool -change ${OLD} @executable_path/../Frameworks/${SONAME} ${EXECUTABLE}
		install_name_tool -id @executable_path/../Frameworks/${SONAME} Subsurface.app/Contents/Frameworks/${SONAME}
	fi
done

# next, replace @rpath references with @executable_path references in Subsurface
RPATH=$(otool -L ${EXECUTABLE} | grep rpath  | cut -d\  -f1 | tr -d "\t" | cut -b 8- )
for i in ${RPATH}; do
	install_name_tool -change @rpath/$i @executable_path/../Frameworks/$i ${EXECUTABLE}
done

