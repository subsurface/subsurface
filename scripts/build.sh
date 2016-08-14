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

# Verify that the Xcode Command Line Tools are installed
if [ $PLATFORM = Darwin ] ; then
	if [ ! -d /usr/include ] ; then
		echo "Error: Xcode Command Line Tools are not installed"
		echo ""
		echo "Please run:"
		echo " xcode-select --install"
		echo "to install them (you'll have to agree to Apple's licensing terms etc), then run build.sh again"
		exit 1;
	fi
fi

# normally this script builds the desktop version in subsurface/build
# if the first argument is "-mobile" then build Subsurface-mobile in subsurface/build-mobile
# if the first argument is "-both" then build both in subsurface/build and subsurface/build-mobile
BUILDGRANTLEE=0
if [ "$1" = "-mobile" ] ; then
	echo "building Subsurface-mobile in subsurface/build-mobile"
	BUILDS=( "MobileExecutable" )
	BUILDDIRS=( "build-mobile" )
	shift
elif [ "$1" = "-both" ] ; then
	echo "building both Subsurface and Subsurface-mobile in subsurface/build and subsurface/build-mobile, respectively"
	BUILDS=( "DesktopExecutable" "MobileExecutable" )
	BUILDDIRS=( "build" "build-mobile" )
	BUILDGRANTLEE=1
	shift
else
	echo "building Subsurface in subsurface/build"
	BUILDS=( "DesktopExecutable" )
	BUILDDIRS=( "build" )
	BUILDGRANTLEE=1
fi

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

mkdir -p build
cd build

if [ ! -f ../configure ] ; then
	autoreconf --install ..
fi
../configure --prefix=$INSTALL_ROOT --disable-examples
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
	if [ -d "$HOME/Qt/5.5" ] ; then
		export CMAKE_PREFIX_PATH=~/Qt/5.5/clang_64/lib/cmake
	elif [ -d "$HOME/Qt/5.6" ] ; then
		export CMAKE_PREFIX_PATH=~/Qt/5.6/clang_64/lib/cmake
	elif [ -d /usr/local/opt/qt5/lib ] ; then
		# Homebrew location for qt5 package
		export CMAKE_PREFIX_PATH=/usr/local/opt/qt5/lib/cmake
	else
		echo "cannot find Qt 5.5 or 5.6 in ~/Qt"
		exit 1
	fi
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

if [ "$BUILDGRANTLEE" = "1" ] ; then
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




# finally, build Subsurface

if [ $PLATFORM = Darwin ] ; then
	SH_LIB_EXT=dylib
else
	SH_LIB_EXT=so
fi

cd $SRC/subsurface
for (( i=0 ; i < ${#BUILDS[@]} ; i++ )) ; do
	SUBSURFACE_EXECUTABLE=${BUILDS[$i]}
	BUILDDIR=${BUILDDIRS[$i]}
	echo "build $SUBSURFACE_EXECUTABLE in $BUILDDIR"

	# pull the plasma-mobile components from upstream if building Subsurface-mobile
	if [ "$SUBSURFACE_EXECUTABLE" = "MobileExecutable" ] ; then
		cd $SRC/subsurface
		bash ./scripts/mobilecomponents.sh
		mkdir -p $SRC/kirigami-build
		cd $SRC/kirigami-build
		cmake $SRC/subsurface/mobile-widgets/qml/kirigami/ -DSTATIC_LIBRARY=ON
		make -j4
	fi

	mkdir -p $SRC/subsurface/$BUILDDIR
	cd $SRC/subsurface/$BUILDDIR
	export CMAKE_PREFIX_PATH="$INSTALL_ROOT/lib/cmake;${CMAKE_PREFIX_PATH}"
	cmake -DCMAKE_BUILD_TYPE=Debug .. \
		-DSUBSURFACE_TARGET_EXECUTABLE=$SUBSURFACE_EXECUTABLE \
		-DLIBGIT2_INCLUDE_DIR=$INSTALL_ROOT/include \
		-DLIBGIT2_LIBRARIES=$INSTALL_ROOT/lib/libgit2.$SH_LIB_EXT \
		-DLIBDIVECOMPUTER_INCLUDE_DIR=$INSTALL_ROOT/include \
		-DLIBDIVECOMPUTER_LIBRARIES=$INSTALL_ROOT/lib/libdivecomputer.a \
		-DMARBLE_INCLUDE_DIR=$INSTALL_ROOT/include \
		-DMARBLE_LIBRARIES=$INSTALL_ROOT/lib/libssrfmarblewidget.$SH_LIB_EXT \
		-DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH \
		-DNO_PRINTING=OFF

	if [ $PLATFORM = Darwin ] ; then
		rm -rf Subsurface.app
		rm -rf Subsurface-mobile.app
	fi

	LIBRARY_PATH=$INSTALL_ROOT/lib make -j4

	if [ $PLATFORM = Darwin ] ; then
		LIBRARY_PATH=$INSTALL_ROOT/lib make install
	fi
done
