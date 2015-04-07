#!/bin/bash
#
# this should be run from the src directory, the layout is supposed to
# look like this:
#.../src/subsurface
#       /libgit2
#       /marble-source
#       /libdivecomputer
#
# the script will build these three libraries from source, even if
# they are installed as part of the host OS since we have seen 
# numerous cases where building with random versions (especially older,
# but sometimes also newer versions than recommended here) will lead
# to all kinds of unnecessary pain

SRC=$(pwd)
if [[ ! -d "subsurface" ]] ; then
	echo "please start this script from the directory containing the Subsurface source directory"
	exit 1
fi

# qmake or qmake-qt5 ?
qmake -v | grep "version 5" > /dev/null 2>&1
if [[ $? -eq 0 ]] ; then
	QMAKE=qmake
else
	qmake-qt5 -v | grep "version 5" > /dev/null 2>&1
	if [[ $? -eq 0 ]] ; then
		QMAKE=qmake-qt5
	else
		echo "can't find a working qmake for Qt5"
		exit 1
	fi
fi

mkdir -p install

# build libgit2

if [ ! -d libgit2 ] ; then
	if [[ $1 = local ]] ; then
		git clone $SRC/../libgit2 libgit2
	else
		git clone git://github.com/libgit2/libgit2
	fi
fi
cd libgit2
git checkout v0.22.0
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$SRC/install -DCMAKE_BUILD_TYPE=Release -DBUILD_CLAR=OFF ..
cmake --build . --target install

cd $SRC

# build libdivecomputer

if [ ! -d libdivecomputer ] ; then
	if [[ $1 = local ]] ; then
		git clone $SRC/../libdivecomputer libdivecomputer
	else
		git clone -b Subsurface-4.4 git://subsurface-divelog.org/libdc libdivecomputer
	fi
fi
cd libdivecomputer
git checkout Subsurface-4.4
if [ ! -f configure ] ; then
	autoreconf --install
	./configure --prefix=$SRC/install
fi
make -j4
make install

cd $SRC

# build libssrfmarblewidget

if [ ! -d marble-source ] ; then
	if [[ $1 = local ]] ; then
		git clone $SRC/../marble-source marble-source
	else
		git clone -b Subsurface-4.4 git://subsurface-divelog.org/marble marble-source
	fi
fi
cd marble-source
git checkout Subsurface-4.4
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DQTONLY=TRUE -DQT5BUILD=ON \
	-DCMAKE_INSTALL_PREFIX=$SRC/install \
	-DBUILD_MARBLE_TESTS=NO \
	-DWITH_DESIGNER_PLUGIN=NO \
	-DBUILD_MARBLE_APPS=NO \
	$SRC/marble-source
cd src/lib/marble
make -j4
make install

cd $SRC/subsurface
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$SRC/install ..
make -j4

