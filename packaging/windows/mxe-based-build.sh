#!/bin/bash

# build Subsurface for Win32
#
# this file assumes that you have installed mxe on your system
# and installed a number of dependencies as well
# it also makes some assumption about the filesystem layout based
# on the way things are setup on my system so I can build Ubuntu PPA,
# OBS and Windows out of the same sources.
# Something like this:
#
# ~/src/win/mxe                    <- current MXE git with Qt5, automake
#      /win/libxml2                <- libxml from git
#      /win/libxslt                <- libxslt from git
#      /win/libzip-0.11.2          <- libzip sources from their latest distribution tar ball
#      /subsurface                 <- current subsurface git
#      /subsurface/libdivecomputer <- appropriate libdc branch
#      /subsurface/marble-source   <- appropriate marble branch
#      /subsurface/libgit2         <- appropriate libgit2 branch
#
# ~/src/win/win32                  <- build directory
#                /libxml2
#                /libxslt
#                /libzip
#                /libgit2
#                /marble
#                /subsurface
#
# then start this script from ~/src/win/win32
#
#  cd ~/src/win/win32
#  bash ../../subsurface/packaging/windows/mxe-based-build.sh installer
#
# this should create the latest daily installer

if [[ ! -d libgit2 || ! -d marble || ! -d subsurface ]] ; then
	echo "Please start this from the right directory"
	exit 1
fi

BASEDIR=$(cd "`dirname $0`/.."; pwd)

echo "Building in $BASEDIR ..."

export PATH=$BASEDIR/mxe/usr/bin:$PATH:$BASEDIR/mxe/usr/i686-w64-mingw32.shared/qt5/bin/

####################
if false; then
####################

# libxml2

cd libxml2 || exit 1
../../libxml2/configure --host=i686-w64-mingw32.shared \
	--without-python \
	--prefix=$BASEDIR/mxe/usr/i686-w64-mingw32.shared/
make -j12
make install
cd ..

# libxslt

cd libxslt || exit 1
../../libxslt/autogen.sh  --host=i686-w64-mingw32.shared \
        --without-python \
        --prefix=$BASEDIR/mxe/usr/i686-w64-mingw32.shared/
make -j12
make install
cd ..

# libzip

cd libzip || exit 1
../../libzip-0.11.2/configure --host=i686-w64-mingw32.shared \
	--enable-static \
	--prefix=$BASEDIR/mxe/usr/i686-w64-mingw32.shared/
make -j12
make install
cd ..

# libgit2:

cd libgit2 || exit 1
cmake -DCMAKE_TOOLCHAIN_FILE=$BASEDIR/mxe/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake \
	-DBUILD_CLAR=OFF \
	$BASEDIR/../subsurface/libgit2
make -j12

# fake install
if [ ! -h include ] ; then
	ln -s $BASEDIR/../subsurface/libgit2/include .
fi
if [ ! -h build ] ; then
	ln -s . build
fi
cd ..

# libdivecomputer (can't figure out how to do out-of tree builds?)

cd libdivecomputer || exit 1
git pull
autoreconf --install
./configure --host=i686-w64-mingw32.shared --enable-static --disable-shared
make -j12
cd ..

# marble:

cd marble
cmake -DCMAKE_TOOLCHAIN_FILE=$BASEDIR/mxe/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake \
	-DCMAKE_PREFIX_PATH=$BASEDIR/mxe/usr/i686-w64-mingw32.shared/qt5 \
 	-DQTONLY=ON -DQT5BUILD=ON \
        -DBUILD_MARBLE_APPS=OFF -DBUILD_MARBLE_EXAMPLES=OFF \
        -DBUILD_MARBLE_TESTS=OFF -DBUILD_MARBLE_TOOLS=OFF \
        -DBUILD_TESTING=OFF -DWITH_DESIGNER_PLUGIN=OFF \
        -DBUILD_WITH_DBUS=OFF $BASEDIR/../subsurface/marble-source
make -j12

# fake install
if [ ! -d include ] ; then
	mkdir include
	cd include
        for i in $(find $BASEDIR/../subsurface/marble-source -name \*.h); do ln -s $i .; done
	ln -s . marble
	cd ..
fi
if [ ! -d lib ] ; then
	mkdir lib
fi
cp src/lib/marble/libssrfmarblewidget.dll lib
cd ..

###############
fi
###############

# subsurface:

cd subsurface || exit 1

if [ ! -d staging ] ; then
	mkdir -p staging/plugins
	cd staging/plugins
	cp -a $BASEDIR/mxe/usr/i686-w64-mingw32.shared/qt5/plugins/iconengines .
	cp -a $BASEDIR/mxe/usr/i686-w64-mingw32.shared/qt5/plugins/imageformats .
	cp -a $BASEDIR/mxe/usr/i686-w64-mingw32.shared/qt5/plugins/platforms .
	cp -a $BASEDIR/mxe/usr/i686-w64-mingw32.shared/qt5/plugins/printsupport .
	cd ../..
fi

export objdump=$BASEDIR/mxe/usr/bin/i686-w64-mingw32.shared-objdump

i686-w64-mingw32.shared-qmake-qt5 \
	LIBMARBLEDEVEL=../marble \
	LIBGIT2DEVEL=../libgit2 CONFIG+=libgit21-api \
	LIBDCDEVEL=../libdivecomputer \
	QMAKE_LRELEASE=$BASEDIR/mxe/usr/i686-w64-mingw32.shared/qt5/bin/lrelease \
	SPECIAL_MARBLE_PREFIX=1 \
	MAKENSIS=i686-w64-mingw32.shared-makensis \
	$BASEDIR/../subsurface/subsurface.pro


make -j12 $@
