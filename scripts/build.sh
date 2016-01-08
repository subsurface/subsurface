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
#
# it installs the libraries and subsurface in the install-root subdirectory
# of the current directory (except on Mac where the Subsurface.app ends up
# in subsurface/build

# create a log file of the build
exec 1> >(tee build.log) 2>&1

SRC=$(pwd)
PLATFORM=$(uname)

# to build Subsurface-mobile on the desktop change this to
# SUBSURFACE_EXECUTABLE=MobileExecutable
SUBSURFACE_EXECUTABLE=DesktopExecutable


if [[ ! -d "subsurface" ]] ; then
	echo "please start this script from the directory containing the Subsurface source directory"
	exit 1
fi

mkdir -p install-root
INSTALL_ROOT=$SRC/install-root

# make sure we find our own packages first (e.g., libgit2 only uses pkg_config to find libssh2)
export PKG_CONFIG_PATH=$INSTALL_ROOT/lib/pkgconfig:$PKG_CONFIG_PATH

echo Building in $SRC, installing in $INSTALL_ROOT

# build libgit2

cd $SRC

if [ ! -d libgit2 ] ; then
	if [[ $1 = local ]] ; then
		git clone $SRC/../libgit2 libgit2
	else
		git clone git://github.com/libgit2/libgit2
	fi
fi
cd libgit2
# let's build with a recent enough version of master for the latest features
git fetch origin
if ! git checkout v0.23.1 ; then
	echo "Can't find the right tag in libgit2 - giving up"
	exit 1
fi
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_ROOT -DCMAKE_BUILD_TYPE=Release -DBUILD_CLAR=OFF ..
make -j4
make install

if [ $PLATFORM = Darwin ] ; then
	# in order for macdeployqt to do its job correctly, we need the full path in the dylib ID
	cd $INSTALL_ROOT/lib
	NAME=$(otool -L libgit2.dylib | grep -v : | head -1 | cut -f1 -d\  | tr -d '\t')
	echo $NAME | grep / > /dev/null 2>&1
	if [ $? -eq 1 ] ; then
		install_name_tool -id "$INSTALL_ROOT/lib/$NAME" "$INSTALL_ROOT/lib/$NAME"
	fi
fi

cd $SRC

# build libdivecomputer

if [ ! -d libdivecomputer ] ; then
	if [[ $1 = local ]] ; then
		git clone $SRC/../libdivecomputer libdivecomputer
	else
		git clone -b Subsurface-branch git://subsurface-divelog.org/libdc libdivecomputer
	fi
fi
cd libdivecomputer
git pull --rebase
if ! git checkout Subsurface-branch ; then
	echo "can't check out the Subsurface-branch branch of libdivecomputer -- giving up"
	exit 1
fi
if [ ! -f configure ] ; then
	autoreconf --install
fi
./configure --prefix=$INSTALL_ROOT
make -j4
make install

cd $SRC

# build libssrfmarblewidget

if [ ! -d marble-source ] ; then
	if [[ $1 = local ]] ; then
		git clone $SRC/../marble-source marble-source
	else
		git clone -b Subsurface-branch git://subsurface-divelog.org/marble marble-source
	fi
fi
cd marble-source
git pull --rebase
if ! git checkout Subsurface-branch ; then
	echo "can't check out the Subsurface-branch branch of marble -- giving up"
	exit 1
fi
mkdir -p build
cd build
if [ $PLATFORM = Darwin ] ; then
	export CMAKE_PREFIX_PATH=~/Qt/5.5/clang_64/lib/cmake
fi
cmake -DCMAKE_BUILD_TYPE=Release -DQTONLY=TRUE -DQT5BUILD=ON \
	-DCMAKE_INSTALL_PREFIX=$INSTALL_ROOT \
	-DBUILD_MARBLE_TESTS=NO \
	-DWITH_DESIGNER_PLUGIN=NO \
	-DBUILD_MARBLE_APPS=NO \
	$SRC/marble-source
cd src/lib/marble
make -j4
make install

if [ $PLATFORM = Darwin ] ; then
	# in order for macdeployqt to do its job correctly, we need the full path in the dylib ID
	cd $INSTALL_ROOT/lib
	NAME=$(otool -L libssrfmarblewidget.dylib | grep -v : | head -1 | cut -f1 -d\  | tr -d '\t' | cut -f3 -d/ )
	echo $NAME | grep / > /dev/null 2>&1
	if [ $? -eq 1 ] ; then
		install_name_tool -id "$INSTALL_ROOT/lib/$NAME" "$INSTALL_ROOT/lib/$NAME"
	fi
fi

if [ "$SUBSURFACE_EXECUTABLE" = "DesktopExecutable" ] ; then
	# build grantlee

	cd $SRC

	if [ ! -d grantlee ] ; then
		if [[ $1 = local ]] ; then
			git clone $SRC/../grantlee grantlee
		else
			git clone git://subsurface-divelog.org/grantlee
		fi
	fi
	cd grantlee
	if ! git checkout v5.0.0 ; then
		echo "can't check out v5.0.0 of grantlee -- giving up"
		exit 1
	fi
	mkdir -p build
	cd build
	cmake -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=$INSTALL_ROOT \
		-DBUILD__TESTS=NO \
		$SRC/grantlee
	make -j4
	make install
fi

# pull the plasma-mobile components from upstream if building Subsurface-mobile
if [ "$SUBSURFACE_EXECUTABLE" = "MobileExecutable" ] ; then
	# now bring in the latest Plasma-mobile mobile components plus a couple of icons that we need
	# first, get the latest from upstream
	# yes, this is a bit overkill as we clone a lot of stuff for just a few files, but this way
	# we stop having to manually merge our code with upstream all the time
	# as we get closer to shipping a production version we'll likely check out specific tags
	# or SHAs from upstream
	cd $SRC
	if [ ! -d plasma-mobile ] ; then
		git clone git://github.com/KDE/plasma-mobile
	fi
	pushd plasma-mobile
	git pull
	popd
	if [ ! -d breeze-icons ] ; then
		git clone git://anongit.kde.org/breeze-icons
	fi
	pushd breeze-icons
	git pull
	popd

	# now copy the components and a couple of icons into plae
	MC=$SRC/subsurface/qt-mobile/qml/mobilecomponents
	PMMC=plasma-mobile/components/mobilecomponents
	BREEZE=breeze-icons

	rm -rf $MC
	mkdir -p $MC/icons
	cp -R $PMMC/qml/* $MC/
	cp $PMMC/fallbacktheme/*qml $MC/

	cp $BREEZE/icons/actions/24/dialog-cancel.svg $MC/icons
	cp $BREEZE/icons/actions/24/distribute-horizontal-x.svg $MC/icons
	cp $BREEZE/icons/actions/24/document-edit.svg $MC/icons
	cp $BREEZE/icons/actions/24/document-save.svg $MC/icons
	cp $BREEZE/icons/actions/24/go-next.svg $MC/icons
	cp $BREEZE/icons/actions/24/go-previous.svg $MC/icons
	cp $BREEZE/icons/actions/16/view-readermode.svg $MC/icons

	echo org.kde.plasma.mobilecomponents synced from upstream
fi

# finally, build Subsurface

if [ $PLATFORM = Darwin ] ; then
	SH_LIB_EXT=dylib
else
	SH_LIB_EXT=so
fi

cd $SRC/subsurface
mkdir -p build
cd build
export CMAKE_PREFIX_PATH=$INSTALL_ROOT/lib/cmake
cmake -DCMAKE_BUILD_TYPE=Debug .. \
	-DSUBSURFACE_TARGET_EXECUTABLE=$SUBSURFACE_EXECUTABLE \
	-DLIBGIT2_INCLUDE_DIR=$INSTALL_ROOT/include \
	-DLIBGIT2_LIBRARIES=$INSTALL_ROOT/lib/libgit2.$SH_LIB_EXT \
	-DLIBDIVECOMPUTER_INCLUDE_DIR=$INSTALL_ROOT/include \
	-DLIBDIVECOMPUTER_LIBRARIES=$INSTALL_ROOT/lib/libdivecomputer.a \
	-DMARBLE_INCLUDE_DIR=$INSTALL_ROOT/include \
	-DMARBLE_LIBRARIES=$INSTALL_ROOT/lib/libssrfmarblewidget.$SH_LIB_EXT \
	-DNO_PRINTING=OFF

if [ $PLATFORM = Darwin ] ; then
	rm -rf Subsurface.app
fi

LIBRARY_PATH=$INSTALL_ROOT/lib make -j4

if [ $PLATFORM = Darwin ] ; then
	LIBRARY_PATH=$INSTALL_ROOT/lib make install
fi
