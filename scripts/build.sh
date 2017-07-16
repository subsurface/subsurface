#!/bin/bash
#
# this should be run from the src directory, the layout is supposed to
# look like this:
#.../src/subsurface
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

# in order to build the dependencies on Mac for release builds (to deal with the macosx-version-min for those
# call this script with -build-deps
if [ "$1" == "-build-deps" ] ; then
	shift
	BUILD_DEPS="1"
fi

# unless you build Qt from source (or at least webkit from source, you won't have webkit installed
# -build-with-webkit tells the script that in fact we can assume that webkit is present (it usually
# is still available on Linux distros)
if [ "$1" == "-build-with-webkit" ] ; then
	shift
	BUILD_WITH_WEBKIT="1"
fi
if [ $PLATFORM = Linux ] ; then
	BUILD_WITH_WEBKIT="1"
fi

# most of these will only be needed with -build-deps on a Mac
CURRENT_LIBZIP="1.2.0"
CURRENT_HIDAPI="hidapi-0.7.0"
CURRENT_LIBCURL="curl-7_54_1"
CURRENT_LIBUSB="v1.0.21"
CURRENT_OPENSSL="OpenSSL_1_1_0f"
CURRENT_LIBSSH2="libssh2-1.8.0"
CURRENT_LIBGIT2="v0.26.0"

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
BUILDMARBLE=0
if [ "$1" = "-mobile" ] ; then
	echo "building Subsurface-mobile in subsurface/build-mobile"
	BUILDS=( "MobileExecutable" )
	BUILDDIRS=( "build-mobile" )
	shift
elif [ "$1" = "-both" ] ; then
	echo "building both Subsurface and Subsurface-mobile in subsurface/build and subsurface/build-mobile, respectively"
	BUILDS=( "DesktopExecutable" "MobileExecutable" )
	BUILDDIRS=( "build" "build-mobile" )
	if [ "$BUILD_WITH_WEBKIT" = "1" ] ; then
		BUILDGRANTLEE=1
		BUILDMARBLE=1
	fi
	shift
else
	echo "building Subsurface in subsurface/build"
	BUILDS=( "DesktopExecutable" )
	BUILDDIRS=( "build" )
	if [ "$BUILD_WITH_WEBKIT" = "1" ] ; then
		BUILDGRANTLEE=1
		BUILDMARBLE=1
	fi
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

# set up the right file name extensions
if [ $PLATFORM = Darwin ] ; then
	SH_LIB_EXT=dylib
else
	SH_LIB_EXT=so

	# check if we need to build libgit2 (and do so if necessary)

	LIBGIT_ARGS=" -DLIBGIT2_DYNAMIC=ON "
	LIBGIT=$(ldconfig -p | grep libgit2\\.so\\. | awk -F. '{ print $NF }')
fi

if [[ $PLATFORM = Darwin || "$LIBGIT" < "24" ]] ; then
	# when building distributable binaries on a Mac, we cannot rely on anything from Homebrew,
	# because that always requires the latest OS (how stupid is that - and they consider it a
	# feature). So we painfully need to build the dependencies ourselves.

	if [ "$BUILD_DEPS" == "1" ] ; then
		if [ ! -d libzip-${CURRENT_LIBZIP} ] ; then
			curl -O https://nih.at/libzip/libzip-${CURRENT_LIBZIP}.tar.gz
			tar xzf libzip-${CURRENT_LIBZIP}.tar.gz
		fi
		cd libzip-${CURRENT_LIBZIP}
		mkdir -p build
		cd build
		../configure CFLAGS="$OLDER_MAC" --prefix=$INSTALL_ROOT
		make -j4
		make install

		cd $SRC

		if [ ! -d hidapi ] ; then
			git clone https://github.com/signal11/hidapi
		fi
		cd hidapi
		# there is no good tag, so just build master
		bash ./bootstrap
		mkdir -p build
		cd build
		CFLAGS="$OLDER_MAC" ../configure --prefix=$INSTALL_ROOT
		make -j4
		make install

		cd $SRC

		if [ ! -d libcurl ] ; then
			git clone https://github.com/curl/curl libcurl
		fi
		cd libcurl
		if ! git checkout $CURRENT_LIBCURL ; then
			echo "Can't find the right tag in libcurl - giving up"
			exit 1
		fi
		bash ./buildconf
		mkdir -p build
		cd build
		CFLAGS="$OLDER_MAC" ../configure --prefix=$INSTALL_ROOT --with-darwinssl \
			--disable-tftp --disable-ftp --disable-ldap --disable-ldaps --disable-imap --disable-pop3 --disable-smtp --disable-gopher --disable-smb --disable-rtsp
		make -j4
		make install

		cd $SRC

		if [ ! -d libusb ] ; then
			git clone https://github.com/libusb/libusb
		fi
		cd libusb
		if ! git checkout $CURRENT_LIBUSB ; then
			echo "Can't find the right tag in libusb - giving up"
			exit 1
		fi
		bash ./bootstrap.sh
		mkdir -p build
		cd build
		CFLAGS="$OLDER_MAC" ../configure --prefix=$INSTALL_ROOT --disable-examples
		make -j4
		make install

		cd $SRC

		if [ ! -d openssl ] ; then
			git clone https://github.com/openssl/openssl
		fi
		cd openssl
		if ! git checkout $CURRENT_OPENSSL ; then
			echo "Can't find the right tag in openssl - giving up"
			exit 1
		fi
		mkdir -p build
		cd build
		../Configure --prefix=$INSTALL_ROOT --openssldir=$INSTALL_ROOT $OLDER_MAC darwin64-x86_64-cc
		make depend
		# all the tests fail because the assume that openssl is already installed. Odd? Still thinks work
		make -j4 -k
		make -k install

		cd $SRC

		if [ ! -d libssh2 ] ; then
			git clone https://github.com/libssh2/libssh2
		fi
		cd libssh2
		if ! git checkout $CURRENT_LIBSSH2 ; then
			echo "Can't find the right tag in libssh2 - giving up"
			exit 1
		fi
		mkdir -p build
		cd build
		cmake $OLDER_MAC_CMAKE -DCMAKE_INSTALL_PREFIX=$INSTALL_ROOT -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF ..
		make -j4
		make install
	else
		# we are getting libusb and hidapi from pkg-config and that goes wrong
		# or more specifically, the way libdivecomputer references
		# the include files goes wrong
		LIBDC_CFLAGS=-I$(dirname $(pkg-config --cflags libusb-1.0 | sed -e 's/^-I//'))
		LIBDC_CFLAGS="${LIBDC_CFLAGS} -I$(dirname $(pkg-config --cflags hidapi | sed -e 's/^-I//'))"
		echo $LIBDC_CFLAGS
	fi

	LIBGIT_ARGS=" -DLIBGIT2_INCLUDE_DIR=$INSTALL_ROOT/include -DLIBGIT2_LIBRARIES=$INSTALL_ROOT/lib/libgit2.$SH_LIB_EXT "

	cd $SRC

	if [ ! -d libgit2 ] ; then
		if [[ $1 = local ]] ; then
			git clone $SRC/../libgit2 libgit2
		else
			git clone https://github.com/libgit2/libgit2.git
		fi
	fi
	cd libgit2
	# let's build with a recent enough version of master for the latest features
	git fetch origin
	if ! git checkout $CURRENT_LIBGIT2 ; then
		echo "Can't find the right tag in libgit2 - giving up"
		exit 1
	fi
	mkdir -p build
	cd build
	cmake $OLDER_MAC_CMAKE -DCMAKE_INSTALL_PREFIX=$INSTALL_ROOT -DCMAKE_BUILD_TYPE=Release -DBUILD_CLAR=OFF ..
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
fi

cd $SRC

# build libdivecomputer

if [ ! -d libdivecomputer ] ; then
	if [[ $1 = local ]] ; then
		git clone $SRC/../libdivecomputer libdivecomputer
	else
		git clone -b Subsurface-branch https://github.com/Subsurface-divelog/libdc.git libdivecomputer
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
	# this is not a typo
	# in some scenarios it appears that autoreconf doesn't copy the
	# ltmain.sh file; running it twice, however, fixes that problem
	autoreconf --install ..
	autoreconf --install ..
fi
CFLAGS="$OLDER_MAC -I$INSTALL_ROOT/include $LIBDC_CFLAGS" ../configure --prefix=$INSTALL_ROOT --disable-examples
make -j4
make install

if [ $PLATFORM = Darwin ] ; then
	if [ -z "$CMAKE_PREFIX_PATH" ] ; then
		# qmake in PATH?
		libdir=`qmake -query QT_INSTALL_LIBS`
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

# build libssrfmarblewidget

if [ $BUILDMARBLE = 1 ]; then
	MARBLE_OPTS="-DMARBLE_INCLUDE_DIR=$INSTALL_ROOT/include \
		-DMARBLE_LIBRARIES=$INSTALL_ROOT/lib/libssrfmarblewidget.$SH_LIB_EXT \
		-DNO_MARBLE=OFF -DNO_USERMANUAL=OFF -DFBSUPPORT=ON"
	if [ ! -d marble-source ] ; then
		if [[ $1 = local ]] ; then
			git clone $SRC/../marble-source marble-source
		else
			git clone -b Subsurface-branch https://github.com/Subsurface-divelog/marble.git marble-source
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

	cmake $OLDER_MAC_CMAKE -DCMAKE_BUILD_TYPE=Release -DQTONLY=TRUE -DQT5BUILD=ON \
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
else
	MARBLE_OPTS="-DNO_MARBLE=ON -DNO_USERMANUAL=ON -DFBSUPPORT=OFF"
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
		-DBUILD__TESTS=NO \
		$SRC/grantlee
	make -j4
	make install
else
	PRINTING="-DNO_PRINTING=ON"
fi




# finally, build Subsurface

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
		$PRINTING $MARBLE_OPTS

	if [ $PLATFORM = Darwin ] ; then
		rm -rf Subsurface.app
		rm -rf Subsurface-mobile.app
	fi

	LIBRARY_PATH=$INSTALL_ROOT/lib make -j4

	if [ $PLATFORM = Darwin ] ; then
		LIBRARY_PATH=$INSTALL_ROOT/lib make install
	fi
done
