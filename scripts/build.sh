#!/bin/bash
#
# this should be run from the src directory which contains the subsurface
# directory; the layout should look like this:
#.../src/subsurface
#
# the script will build Subsurface and libdivecomputer (plus some other
# dependencies if requestsed) from source.
#
# it installs the libraries and subsurface in the install-root subdirectory
# of the current directory (except on Mac where the Subsurface.app ends up
# in subsurface/build

# create a log file of the build
exec 1> >(tee build.log) 2>&1

SRC=$(pwd)
PLATFORM=$(uname)

BTSUPPORT="ON"

# deal with all the command line arguments
while [[ $# -gt 0 ]] ; do
	arg="$1"
	case $arg in
		-no-bt)
			# force Bluetooth support off
			BTSUPPORT="OFF"
			;;
		-build-deps)
			# in order to build the dependencies on Mac for release builds (to deal with the macosx-version-min for those
			# call this script with -build-deps
			BUILD_DEPS="1"
			;;
		-build-with-webkit)
			# unless you build Qt from source (or at least webkit from source, you won't have webkit installed
			# -build-with-webkit tells the script that in fact we can assume that webkit is present (it usually
			# is still available on Linux distros)
			BUILD_WITH_WEBKIT="1"
			;;
		-mobile)
			# we are building Subsurface-mobile
			# Note that this will run natively on the host OS.
			# To cross build for Android or iOS (including simulator) 
			# use the scripts in packaging/xxx
			BUILD_MOBILE="1"
			;;
		-desktop)
			# we are building Subsurface
			BUILD_DESKTOP="1"
			;;
		-both)
			# we are building Subsurface and Subsurface-mobile
			BUILD_MOBILE="1"
			BUILD_DESKTOP="1"
			;;
		-create-appdir)
			# we are building an AppImage as by product
			CREATE_APPDIR="1"
			;;
		-skip-googlemaps)
			# hack for Travix Mac build
			SKIP_GOOGLEMAPS="1"
			;;
		*)
			echo "Unknown command line argument $arg"
			;;
	esac
	shift
done

# Verify that the Xcode Command Line Tools are installed
if [ $PLATFORM = Darwin ] ; then
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
	OLDER_MAC="-mmacosx-version-min=${BASESDK} -isysroot${SDKROOT}/MacOSX${BASESDK}.sdk"
	OLDER_MAC_CMAKE="-DCMAKE_OSX_DEPLOYMENT_TARGET=${BASESDK} -DCMAKE_OSX_SYSROOT=${SDKROOT}/MacOSX${BASESDK}.sdk/"
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

if [ "$BUILD_MOBILE" = "1" ] ; then
	echo "building Subsurface-mobile in subsurface/build-mobile"
	BUILDS=( "MobileExecutable" )
	BUILDDIRS=( "build-mobile" )
else
	# if no options are given, build Subsurface
	BUILD_DESKTOP="1"
fi

if [ "$BUILD_DESKTOP" = "1" ] ; then
	echo "building Subsurface in subsurface/build"
	BUILDS+=( "DesktopExecutable" )
	BUILDDIRS+=( "build" )
	if [ "$BUILD_WITH_WEBKIT" = "1" ] ; then
		BUILDGRANTLEE=1
	fi
fi

if [[ ! -d "subsurface" ]] ; then
	echo "please start this script from the directory containing the Subsurface source directory"
	exit 1
fi

mkdir -p install-root
INSTALL_ROOT=$SRC/install-root
export INSTALL_ROOT

# make sure we find our own packages first (e.g., libgit2 only uses pkg_config to find libssh2)
export PKG_CONFIG_PATH=$INSTALL_ROOT/lib/pkgconfig:$PKG_CONFIG_PATH

echo Building in $SRC, installing in $INSTALL_ROOT

# find qmake
if [ ! -z $CMAKE_PREFIX_PATH ] ; then
	QMAKE=$CMAKE_PREFIX_PATH/../../bin/qmake
else
	hash qmake > /dev/null && QMAKE=qmake
	[ -z $QMAKE ] && hash qmake-qt5 > /dev/null && QMAKE=qmake-qt5
	[ -z $QMAKE ] && echo "cannot find qmake" && exit 1
fi

# on Debian and Ubuntu based systems, the private QtLocation and
# QtPositioning headers aren't bundled. Download them if necessary.
if [ $PLATFORM = Linux ] ; then
	QT_HEADERS_PATH=`$QMAKE -query QT_INSTALL_HEADERS`
	QT_VERSION=`$QMAKE -query QT_VERSION`

	if [ ! -d "$QT_HEADERS_PATH/QtLocation/$QT_VERSION/QtLocation/private" ] &&
           [ ! -d $INSTALL_ROOT/include/QtLocation/private ] ; then
		echo "Missing private Qt headers for $QT_VERSION; downloading them..."

		QTLOC_GIT=./qtlocation_git
		QTLOC_PRIVATE=$INSTALL_ROOT/include/QtLocation/private
		QTPOS_PRIVATE=$INSTALL_ROOT/include/QtPositioning/private

		rm -rf $QTLOC_GIT > /dev/null 2>&1
		rm -rf $INSTALL_ROOT/include/QtLocation > /dev/null 2>&1
		rm -rf $INSTALL_ROOT/include/QtPositioning > /dev/null 2>&1

		git clone --branch v$QT_VERSION git://code.qt.io/qt/qtlocation.git --depth=1 $QTLOC_GIT

		mkdir -p $QTLOC_PRIVATE
		cd $QTLOC_GIT/src/location
		find -name '*_p.h' | xargs cp -t $QTLOC_PRIVATE
		cd $SRC

		mkdir -p $QTPOS_PRIVATE
		cd $QTLOC_GIT/src/positioning
		find -name '*_p.h' | xargs cp -t $QTPOS_PRIVATE
		cd $SRC

		echo "* cleanup..."
		rm -rf $QTLOC_GIT > /dev/null 2>&1
	fi
fi

# set up the right file name extensions
if [ $PLATFORM = Darwin ] ; then
	SH_LIB_EXT=dylib
	pkg-config --exists libgit2 && LIBGIT=$(pkg-config --modversion libgit2 | cut -d. -f2)
else
	SH_LIB_EXT=so

	LIBGIT_ARGS=" -DLIBGIT2_DYNAMIC=ON "
	# check if we need to build libgit2 (and do so if necessary)
	# first check pkgconfig (that will capture our own local build if
	# this script has been run before)
	if pkg-config --exists libgit2 ; then
		LIBGIT=$(pkg-config --modversion libgit2 | cut -d. -f2)
	fi
	if [[ "$LIBGIT" < "24" ]] ; then
		# maybe there's a system version that's new enough?
		LIBGIT=$(ldconfig -p | grep libgit2\\.so\\. | awk -F. '{ print $NF }')
	fi
fi

if [[ "$LIBGIT" < "24" ]] ; then
	LIBGIT_ARGS=" -DLIBGIT2_INCLUDE_DIR=$INSTALL_ROOT/include -DLIBGIT2_LIBRARIES=$INSTALL_ROOT/lib/libgit2.$SH_LIB_EXT "

	cd $SRC

	./subsurface/scripts/get-dep-lib.sh single . libgit2
	pushd libgit2
	mkdir -p build
	cd build
	cmake $OLDER_MAC_CMAKE -DCMAKE_INSTALL_PREFIX=$INSTALL_ROOT -DCMAKE_BUILD_TYPE=Release -DBUILD_CLAR=OFF ..
	make -j4
	make install
	popd

	if [ $PLATFORM = Darwin ] ; then
		# in order for macdeployqt to do its job correctly, we need the full path in the dylib ID
		cd $INSTALL_ROOT/lib
		NAME=$(otool -L libgit2.dylib | grep -v : | head -1 | cut -f1 -d\  | tr -d '\t')
		echo $NAME | if grep / > /dev/null 2>&1 ; then
			install_name_tool -id "$INSTALL_ROOT/lib/$NAME" "$INSTALL_ROOT/lib/$NAME"
		fi
	fi
fi

if [[ $PLATFORM = Darwin && "$BUILD_DEPS" == "1" ]] ; then
	# when building distributable binaries on a Mac, we cannot rely on anything from Homebrew,
	# because that always requires the latest OS (how stupid is that - and they consider it a
	# feature). So we painfully need to build the dependencies ourselves.

	./subsurface/scripts/get-dep-lib.sh single . libzip
	pushd libzip
	mkdir -p build
	cd build
	../configure CFLAGS="$OLDER_MAC" --prefix=$INSTALL_ROOT
	make -j4
	make install
	popd


	./subsurface/scripts/get-dep-lib.sh single . hidapi
	pushd hidapi
	# there is no good tag, so just build master
	bash ./bootstrap
	mkdir -p build
	cd build
	CFLAGS="$OLDER_MAC" ../configure --prefix=$INSTALL_ROOT
	make -j4
	make install
	popd

	./subsurface/scripts/get-dep-lib.sh single . libcurl
	pushd libcurl
	bash ./buildconf
	mkdir -p build
	cd build
	CFLAGS="$OLDER_MAC" ../configure --prefix=$INSTALL_ROOT --with-darwinssl \
		--disable-tftp --disable-ftp --disable-ldap --disable-ldaps --disable-imap --disable-pop3 --disable-smtp --disable-gopher --disable-smb --disable-rtsp
	make -j4
	make install
	popd

	./subsurface/scripts/get-dep-lib.sh single . libusb
	pushd libusb
	bash ./bootstrap.sh
	mkdir -p build
	cd build
	CFLAGS="$OLDER_MAC" ../configure --prefix=$INSTALL_ROOT --disable-examples
	make -j4
	make install
	popd

	./subsurface/scripts/get-dep-lib.sh single . openssl
	pushd openssl
	mkdir -p build
	cd build
	../Configure --prefix=$INSTALL_ROOT --openssldir=$INSTALL_ROOT $OLDER_MAC darwin64-x86_64-cc
	make depend
	# all the tests fail because the assume that openssl is already installed. Odd? Still thinks work
	make -j4 -k
	make -k install
	popd

	./subsurface/scripts/get-dep-lib.sh single . libssh2
	pushd libssh2
	mkdir -p build
	cd build
	cmake $OLDER_MAC_CMAKE -DCMAKE_INSTALL_PREFIX=$INSTALL_ROOT -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF ..
	make -j4
	make install
	popd
else
	# we are getting libusb and hidapi from pkg-config and that goes wrong
	# or more specifically, the way libdivecomputer references
	# the include files goes wrong
	pkg-config --exists libusb-1.0 && LIBDC_CFLAGS=-I$(dirname $(pkg-config --cflags libusb-1.0 | sed -e 's/^-I//'))
	pkg-config --exists hidapi && LIBDC_CFLAGS="${LIBDC_CFLAGS} -I$(dirname $(pkg-config --cflags hidapi | sed -e 's/^-I//'))"
fi


cd $SRC

# build libdivecomputer

cd subsurface

if [ ! -d libdivecomputer/src ] ; then
	git submodule init
	git submodule update --recursive
fi

cd libdivecomputer

mkdir -p build
cd build

if [ ! -f ../configure ] ; then
	# this is not a typo
	# in some scenarios it appears that autoreconf doesn't copy the
	# ltmain.sh file; running it twice, however, fixes that problem
	autoreconf --install ..
	autoreconf --install ..
fi
CFLAGS="$OLDER_MAC -I$INSTALL_ROOT/include $LIBDC_CFLAGS" ../configure --prefix=$INSTALL_ROOT --disable-examples
if [ $PLATFORM = Darwin ] ; then
	# it seems that on my Mac some of the configure tests for libdivecomputer
	# pass even though the feature tested for is actually missing
	# let's hack around that
	sed -i .bak 's/^#define HAVE_CLOCK_GETTIME 1/\/* #undef HAVE_CLOCK_GETTIME *\//' config.h
	for i in $(find . -name Makefile)
	do
		sed -i .bak 's/-Wrestrict//;s/-Wno-unused-but-set-variable//' $i
	done
fi
make -j4
make install

if [ $PLATFORM = Darwin ] ; then
	if [ -z "$CMAKE_PREFIX_PATH" ] ; then
		libdir=`$QMAKE -query QT_INSTALL_LIBS`
		if [ $? -eq 0 ]; then
			export CMAKE_PREFIX_PATH=$libdir/cmake
		elif [ -d "$HOME/Qt/5.9.1" ] ; then
			export CMAKE_PREFIX_PATH=~/Qt/5.9.1/clang_64/lib/cmake
		elif [ -d "$HOME/Qt/5.9" ] ; then
			export CMAKE_PREFIX_PATH=~/Qt/5.9/clang_64/lib/cmake
		elif [ -d "$HOME/Qt/5.8" ] ; then
			export CMAKE_PREFIX_PATH=~/Qt/5.8/clang_64/lib/cmake
		elif [ -d "$HOME/Qt/5.7" ] ; then
			export CMAKE_PREFIX_PATH=~/Qt/5.7/clang_64/lib/cmake
		elif [ -d "$HOME/Qt/5.6" ] ; then
			export CMAKE_PREFIX_PATH=~/Qt/5.6/clang_64/lib/cmake
		elif [ -d "$HOME/Qt/5.5" ] ; then
			export CMAKE_PREFIX_PATH=~/Qt/5.5/clang_64/lib/cmake
		elif [ -d /usr/local/opt/qt5/lib ] ; then
			# Homebrew location for qt5 package
			export CMAKE_PREFIX_PATH=/usr/local/opt/qt5/lib/cmake
		else
			echo "cannot find Qt 5.5 or newer in ~/Qt"
			exit 1
		fi
	fi
fi

cd $SRC

if [ "$BUILD_WITH_WEBKIT" = "1" ]; then
	EXTRA_OPTS="-DNO_USERMANUAL=OFF -DFBSUPPORT=ON"
else
	EXTRA_OPTS="-DNO_USERMANUAL=ON -DFBSUPPORT=OFF"
fi

if [ "$BUILDGRANTLEE" = "1" ] ; then
	# build grantlee
	PRINTING="-DNO_PRINTING=OFF"

	cd $SRC

	if [ ! -d grantlee ] ; then
		if [[ $1 = local ]] ; then
			git clone $SRC/../grantlee grantlee
		else
			git clone https://github.com/steveire/grantlee.git
		fi
	fi
	cd grantlee
	if ! git checkout v5.0.0 ; then
		echo "can't check out v5.0.0 of grantlee -- giving up"
		exit 1
	fi
	mkdir -p build
	cd build
	cmake $OLDER_MAC_CMAKE -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=$INSTALL_ROOT \
		-DBUILD_TESTS=NO \
		$SRC/grantlee
	make -j4
	make install
else
	PRINTING="-DNO_PRINTING=ON"
fi

if [ "$SKIP_GOOGLEMAPS" != "1" ] ; then
	# build the googlemaps map plugin

	cd $SRC
	if [ ! -d googlemaps ] ; then
		if [[ $1 = local ]] ; then
			git clone $SRC/../googlemaps googlemaps
		else
			git clone https://github.com/Subsurface-divelog/googlemaps.git
		fi
	fi
	cd googlemaps
	git checkout master
	git pull --rebase

	mkdir -p build
	mkdir -p J10build
	cd build
	$QMAKE -query
	$QMAKE "INCLUDEPATH=$INSTALL_ROOT/include" ../googlemaps.pro
	# on Travis the compiler doesn't support c++1z, yet qmake adds that flag;
	# since things compile fine with c++11, let's just hack that away
	# similarly, don't use -Wdata-time
	mv Makefile Makefile.bak
	cat Makefile.bak | sed -e 's/std=c++1z/std=c++11/g ; s/-Wdate-time//' > Makefile
	make -j4
	make install
fi

# finally, build Subsurface

set -x

cd $SRC/subsurface
for (( i=0 ; i < ${#BUILDS[@]} ; i++ )) ; do
	SUBSURFACE_EXECUTABLE=${BUILDS[$i]}
	BUILDDIR=${BUILDDIRS[$i]}
	echo "build $SUBSURFACE_EXECUTABLE in $BUILDDIR"

	# pull the plasma-mobile components from upstream if building Subsurface-mobile
	if [ "$SUBSURFACE_EXECUTABLE" = "MobileExecutable" ] ; then
		cd $SRC/subsurface
		bash ./scripts/mobilecomponents.sh
	fi

	mkdir -p $SRC/subsurface/$BUILDDIR
	cd $SRC/subsurface/$BUILDDIR
	export CMAKE_PREFIX_PATH="$INSTALL_ROOT/lib/cmake;${CMAKE_PREFIX_PATH}"
	cmake -DCMAKE_BUILD_TYPE=Debug .. \
		-DSUBSURFACE_TARGET_EXECUTABLE=$SUBSURFACE_EXECUTABLE \
		${LIBGIT_ARGS} \
		-DLIBDIVECOMPUTER_INCLUDE_DIR=$INSTALL_ROOT/include \
		-DLIBDIVECOMPUTER_LIBRARIES=$INSTALL_ROOT/lib/libdivecomputer.a \
		-DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH \
		-DBTSUPPORT=${BTSUPPORT} \
		-DCMAKE_INSTALL_PREFIX=${INSTALL_ROOT} \
		-DLIBGIT2_FROM_PKGCONFIG=ON \
		-DFORCE_LIBSSH=OFF \
		$PRINTING $EXTRA_OPTS

	if [ $PLATFORM = Darwin ] ; then
		rm -rf Subsurface.app
		rm -rf Subsurface-mobile.app
	fi

	LIBRARY_PATH=$INSTALL_ROOT/lib make -j4
	LIBRARY_PATH=$INSTALL_ROOT/lib make install

	if [ "$CREATE_APPDIR" = "1" ] ; then
		# if we create an AppImage this makes gives us a sane starting point
		cd $SRC
		mkdir -p ./appdir
		mkdir -p appdir/usr/share/metainfo
		mkdir -p appdir/usr/share/icons/hicolor/256x256/apps
		cp -r ./install-root/* ./appdir/usr
		cp subsurface/appdata/subsurface.appdata.xml appdir/usr/share/metainfo/
		cp subsurface/icons/subsurface-icon.png appdir/usr/share/icons/hicolor/256x256/apps/
	fi
done
