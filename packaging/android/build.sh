#!/bin/bash
#
# Filesystem layout considerations...
# for this explanation I assume that your Subsurface sources are in
# ~/src/subsurface
#
# You need to have a version of Qt that contains the Android bits
# installed. You should be able to find the correct installer for
# Linux or Mac here:
# http://download.qt.io/official_releases/qt/
# make sure you pick one with 'android' in its name.
#
# Install this wherever you want - and then have a link named ~/src/Qt that
# points to it.
# So let's assume that you are installing the package above in ~/Qt
# (which I think is the default location), then simply do
# cd ~/src
# ln -s ~/Qt Qt
#
# you also need to have the current Android SDK and NDK installed under ~/src
#
# Or just set QT5_ANDROID, ANDROID_SDK_ROOT and ANDROID_NDK_ROOT to where ever you have them.
#
set -eu
PLATFORM=$(uname)
# (trick to get the absolute path, either if we're called with a
# absolute path or a relative path)
pushd "$(dirname "$0")/../../"
export SUBSURFACE_SOURCE=$PWD
popd

# Set build defaults
# is this a release or debug build
BUILD_TYPE=Debug
# Build-nr in the android manifest.
BUILD_NR=0
# Should we build the desktop ui or the mobile ui?
SUBSURFACE_DESKTOP=OFF
# Which arch should we build for?
ARCH=arm
# Read build variables
source subsurface/packaging/android/variables.sh 

QUICK=""

while [ "$#" -gt 0 ] ; do
	case "$1" in
		Release|release)
			shift
			BUILD_TYPE=Release
			;;
		Debug|debug)
			# this is the default - still need to eat the argument if given
			BUILD_TYPE=Debug
			shift
			;;
		-buildnr)
			shift
			BUILD_NR=$1
			shift
			;;
		desktop)
			SUBSURFACE_DESKTOP=ON
			shift
			;;
		arm|x86)
			ARCH=$1
			shift
			;;
		-quick)
			QUICK="1"
			shift
			;;
		--)
			shift
			break
			;;
		*)
			echo "Unknown argument $1"
			exit 1
			;;
	esac
done

# Its needed by all sub-cmds
export ARCH

# Configure where we can find things here
export ANDROID_NDK_ROOT=${ANDROID_NDK_ROOT-$SUBSURFACE_SOURCE/../${ANDROID_NDK}}

if [ -n "${QT5_ANDROID+X}" ] ; then
	echo "Using Qt5 in $QT5_ANDROID"
elif [ -d "$SUBSURFACE_SOURCE/../Qt/${LATEST_QT}" ] ; then
	export QT5_ANDROID=$SUBSURFACE_SOURCE/../Qt/${LATEST_QT}
elif [ -d "$SUBSURFACE_SOURCE/../Qt/5.9.3" ] ; then
	export QT5_ANDROID=$SUBSURFACE_SOURCE/../Qt/5.9.3
elif [ -d "$SUBSURFACE_SOURCE/../Qt/5.9.1" ] ; then
	export QT5_ANDROID=$SUBSURFACE_SOURCE/../Qt/5.9.1
elif [ -d "$SUBSURFACE_SOURCE/../Qt/5.9" ] ; then
	export QT5_ANDROID=$SUBSURFACE_SOURCE/../Qt/5.9
elif [ -d "$SUBSURFACE_SOURCE/../Qt/5.8" ] ; then
	export QT5_ANDROID=$SUBSURFACE_SOURCE/../Qt/5.8
else
	echo "Cannot find Qt 5.8 or newer under $SUBSURFACE_SOURCE/../Qt"
	exit 1
fi

if [ "$PLATFORM" = "Darwin" ] ; then
	export ANDROID_SDK_ROOT=${ANDROID_SDK_ROOT-$SUBSURFACE_SOURCE/../android-sdk-macosx}
	export ANDROID_NDK_HOST=darwin-x86_64
else
	export ANDROID_SDK_ROOT=${ANDROID_SDK_ROOT-$SUBSURFACE_SOURCE/../android-sdk-linux}
	export ANDROID_NDK_HOST=linux-x86_64
fi

if [ "$ARCH" = "arm" ] ; then
	QT_ARCH=armv7
	BUILDCHAIN=arm-linux-androideabi
	OPENSSL_MACHINE=armv7
elif [ "$ARCH" = "x86" ] ; then
	QT_ARCH=$ARCH
	BUILDCHAIN=i686-linux-android
	OPENSSL_MACHINE=i686
fi

# Verify Qt install and adjust for single-arch Qt install layout
# (e.g. when building Qt from scratch)
export QT5_ANDROID_CMAKE
if [ -d "${QT5_ANDROID}/android_${QT_ARCH}/lib/cmake" ] ; then
	export QT5_ANDROID_CMAKE=$QT5_ANDROID/android_${QT_ARCH}/lib/cmake
elif [ -d "${QT5_ANDROID}/lib/cmake" ] ; then
	export QT5_ANDROID_CMAKE=$QT5_ANDROID/lib/cmake
else
	echo "Cannot find Qt cmake configuration"
	exit 1
fi

if [ ! -e ndk-"$ARCH" ] ; then
	"$ANDROID_NDK_ROOT/build/tools/make_standalone_toolchain.py" --arch="$ARCH" --install-dir=ndk-"$ARCH" --api=$ANDROID_PLATFORM_LEVEL
fi
export BUILDROOT=$PWD
export PATH=${BUILDROOT}/ndk-$ARCH/bin:$PATH
export PREFIX=${BUILDROOT}/ndk-$ARCH/sysroot/usr
export PKG_CONFIG_LIBDIR=$PREFIX/lib/pkgconfig
export CC=${BUILDROOT}/ndk-$ARCH/bin/${BUILDCHAIN}-gcc
export CXX=${BUILDROOT}/ndk-$ARCH/bin/${BUILDCHAIN}-g++
# autoconf seems to get lost without this
export SYSROOT=${BUILDROOT}/ndk-$ARCH/sysroot
export CFLAGS=--sysroot=${SYSROOT}
export CPPFLAGS=--sysroot=${SYSROOT}
export CXXFLAGS=--sysroot=${SYSROOT}
# Junk needed for qt-android-cmake
export ANDROID_STANDALONE_TOOLCHAIN=${BUILDROOT}/ndk-$ARCH
if [ "$PLATFORM" = "Darwin" ] ; then
	JAVA_HOME=$(/usr/libexec/java_home)
	export JAVA_HOME
else
	export JAVA_HOME=/usr
fi

# find qmake
QMAKE=$QT5_ANDROID/android_$QT_ARCH/bin/qmake
echo $QMAKE
$QMAKE -query

# if we are just doing a quick rebuild, don't bother with any of the dependencies

if [ "$QUICK" = "" ] ; then

	# don't adjust indentation to make this a more reasonable commit
	# build google maps plugin
	"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . googlemaps
	# find qmake
	QMAKE=$QT5_ANDROID/android_$QT_ARCH/bin/qmake
	$QMAKE -query
	QT_PLUGINS_PATH=$($QMAKE -query QT_INSTALL_PLUGINS)
	GOOGLEMAPS_BIN=libqtgeoservices_googlemaps.so
	if [ ! -e "$QT_PLUGINS_PATH"/geoservices/$GOOGLEMAPS_BIN ] || [ googlemaps/.git/HEAD -nt "$QT_PLUGINS_PATH"/geoservices/$GOOGLEMAPS_BIN ] ; then
	    mkdir -p googlemaps-build-"$ARCH"
	    pushd googlemaps-build-"$ARCH"
	    $QMAKE ../googlemaps/googlemaps.pro
	    # on Travis the compiler doesn't support c++1z, yet qmake adds that flag;
	    # since things compile fine with c++11, let's just hack that away
	    # similarly, don't use -Wdata-time
	    sed -i.bak -e 's/std=c++1z/std=c++11/g ; s/-Wdate-time//' Makefile
	    make -j4
	    $QMAKE -install qinstall -exe $GOOGLEMAPS_BIN "$QT_PLUGINS_PATH"/geoservices/$GOOGLEMAPS_BIN
	    popd
	fi

	"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . sqlite
	if [ ! -e "$PKG_CONFIG_LIBDIR/sqlite3.pc" ] ; then
		mkdir -p sqlite-build-"$ARCH"
		pushd sqlite-build-"$ARCH"
		../sqlite/configure --host=${BUILDCHAIN} --prefix="$PREFIX" --enable-static --disable-shared
		make
		make install
		popd
	fi

	"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . libxml2
	if [ ! -e libxml2/configure ] ; then
		pushd libxml2
		autoreconf --install
		popd
	fi
	if [ ! -e "$PKG_CONFIG_LIBDIR/libxml-2.0.pc" ] ; then
		mkdir -p libxml2-build-"$ARCH"
		pushd libxml2-build-"$ARCH"
		../libxml2/configure --host=${BUILDCHAIN} --prefix="$PREFIX" --without-python --without-iconv --enable-static --disable-shared
		perl -pi -e 's/runtest\$\(EXEEXT\)//' Makefile
		perl -pi -e 's/testrecurse\$\(EXEEXT\)//' Makefile
		make
		make install
		popd
	fi

	"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . libxslt
	if [ ! -e libxslt/configure ] ; then
		pushd libxslt
		autoreconf --install
		popd
	fi
	if [ ! -e "$PKG_CONFIG_LIBDIR/libxslt.pc" ] ; then
		mkdir -p libxslt-build-"$ARCH"
		pushd libxslt-build-"$ARCH"
		../libxslt/configure --host=${BUILDCHAIN} --prefix="$PREFIX" --with-libxml-prefix="$PREFIX" --without-python --without-crypto --enable-static --disable-shared
		make
		make install
		popd
	fi
	
	"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . libzip
	if [ ! -e "$PKG_CONFIG_LIBDIR/libzip.pc" ] ; then
		# libzip expects a predefined macro that isn't there for our compiler
		pushd libzip
		git reset --hard
		sed -i 's/SIZEOF_SIZE_T/__SIZEOF_SIZE_T__/g' lib/compat.h
		# also, don't deal with manuals and bzip2
		sed -i 's/ADD_SUBDIRECTORY(man)//;s/FIND_PACKAGE(BZip2)/# FIND_PACKAGE(BZip2)/' CMakeLists.txt
		popd
		mkdir -p libzip-build-"$ARCH"
		pushd libzip-build-"$ARCH"
		cmake \
			-DCMAKE_C_COMPILER="$CC" \
			-DCMAKE_LINKER="$CC" \
			-DCMAKE_INSTALL_PREFIX="$PREFIX" \
			-DCMAKE_INSTALL_LIBDIR="lib" \
			-DBUILD_SHARED_LIBS=OFF \
			../libzip/
		make
		make install
		popd
	fi

	"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . openssl
	if [ ! -e "$PKG_CONFIG_LIBDIR/libssl.pc" ] ; then
		mkdir -p openssl-build-"$ARCH"
		cp -r openssl/* openssl-build-"$ARCH"
		pushd openssl-build-"$ARCH"
		perl -pi -e 's/install: all install_docs install_sw/install: install_docs install_sw/g' Makefile.org
		# Use env to make all these temporary, so they don't pollute later builds.
		env SYSTEM=android \
			CROSS_COMPILE=${BUILDCHAIN}- \
			MACHINE=$OPENSSL_MACHINE \
			HOSTCC=gcc \
			CC=gcc \
			ANDROID_DEV="$PREFIX" \
			bash -x ./config shared no-ssl2 no-ssl3 no-comp no-hw no-engine --openssldir="$PREFIX"
	#	sed -i.bak -e 's/soname=\$\$SHLIB\$\$SHLIB_SOVER\$\$SHLIB_SUFFIX/soname=\$\$SHLIB/g' Makefile.shared
		make depend
		make
		# now fix the reference to libcrypto.so.1.0.0 to be just to libcrypto.so
		perl -pi -e 's/libcrypto.so.1.0.0/libcrypto.so\x00\x00\x00\x00\x00\x00/' libssl.so.1.0.0
		make install_sw
		popd
	fi

	"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . libgit2
	if [ ! -e "$PKG_CONFIG_LIBDIR/libgit2.pc" ] ; then
		# We don't want to find the HTTP_Parser package of the build host by mistake
		perl -pi -e 's/FIND_PACKAGE\(HTTP_Parser\)/#FIND_PACKAGE(HTTP_Parser)/' libgit2/CMakeLists.txt
		mkdir -p libgit2-build-"$ARCH"
		pushd libgit2-build-"$ARCH"
		cmake \
			-DCMAKE_C_COMPILER="$CC" \
			-DCMAKE_LINKER="$CC" \
			-DBUILD_CLAR=OFF -DBUILD_SHARED_LIBS=OFF \
			-DCMAKE_INSTALL_PREFIX="$PREFIX" \
			-DCURL=OFF \
			-DUSE_SSH=OFF \
			-DOPENSSL_SSL_LIBRARY="$PREFIX"/lib/libssl.so \
			-DOPENSSL_CRYPTO_LIBRARY="$PREFIX"/lib/libcrypto.so \
			-DOPENSSL_INCLUDE_DIR="$PREFIX"/include/openssl \
			-D_OPENSSL_VERSION="${OPENSSL_VERSION}" \
			../libgit2/
		make
		make install
		# Patch away pkg-config dependency to zlib, its there, i promise
		perl -pi -e 's/^(Requires.private:.*)zlib(.*)$/$1 $2/' "$PKG_CONFIG_LIBDIR"/libgit2.pc
		popd
	fi

	"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . libusb
	if ! grep -q libusb_set_android_open_callback libusb/libusb/libusb.h ; then
		# Patch in our libusb callback
		pushd libusb
		patch -p1 < "$SUBSURFACE_SOURCE"/packaging/android/patches/libusb-android.patch
		popd
	fi
	if [ ! -e libusb/configure ] ; then
		pushd libusb
		mkdir m4
		autoreconf -i
		popd
	fi
	if [ ! -e "$PKG_CONFIG_LIBDIR/libusb-1.0.pc" ] ; then
		mkdir -p libusb-build-"$ARCH"
		pushd libusb-build-"$ARCH"
		../libusb/configure --host=${BUILDCHAIN} --prefix="$PREFIX" --enable-static --disable-shared --disable-udev --enable-system-log
		# --enable-debug-log
		make
		make install
		popd
	fi

	"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . libftdi1
	if [ ! -e "$PKG_CONFIG_LIBDIR/libftdi1.pc" ] && [ "$PLATFORM" != "Darwin" ] ; then
		mkdir -p libftdi1-build-"$ARCH"
		pushd libftdi1-build-"$ARCH"
		cmake ../libftdi1 -DCMAKE_C_COMPILER="$CC" -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_PREFIX_PATH="$PREFIX" -DSTATICLIBS=ON -DPYTHON_BINDINGS=OFF -DDOCUMENTATION=OFF -DFTDIPP=OFF -DBUILD_TESTS=OFF -DEXAMPLES=OFF -DFTDI_EEPROM=OFF
		make
		make install
		popd
	fi
	# Blast away the shared version to force static linking
	if [ -e "$PREFIX/lib/libftdi1.so" ] ; then
		rm "$PREFIX"/lib/libftdi1.so*
	fi

fi # QUICK

pushd "$SUBSURFACE_SOURCE"
git submodule update --recursive
popd
CURRENT_SHA=$(cd "$SUBSURFACE_SOURCE"/libdivecomputer ; git describe)
PREVIOUS_SHA=$(cat "libdivecomputer-${ARCH}.SHA" 2>/dev/null || echo)
if [ ! "$CURRENT_SHA" = "$PREVIOUS_SHA" ] || [ ! -e "$PKG_CONFIG_LIBDIR/libdivecomputer.pc" ] ; then
	mkdir -p libdivecomputer-build-"$ARCH"
	pushd libdivecomputer-build-"$ARCH"
	"$SUBSURFACE_SOURCE"/libdivecomputer/configure --host=${BUILDCHAIN} --prefix="$PREFIX" --enable-static --disable-shared --enable-examples=no
	make
	make install
	popd
	echo "$CURRENT_SHA" > "libdivecomputer-${ARCH}.SHA"
fi

"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . qt-android-cmake
# the Qt Android cmake addon runs androiddeployqt with '--verbose' which
# is, err, rather verbose. Let's not do that.
sed -i -e 's/--verbose//' qt-android-cmake/AddQtAndroidApk.cmake

# Should we build the mobile ui or the desktop ui?
# doing this backwards in order not to break people's setup
if [ "$SUBSURFACE_DESKTOP" = "ON" ] ; then
	SUBSURFACE_MOBILE=
else
	SUBSURFACE_MOBILE=ON
fi

# if we are building Subsurface-mobile and this isn't just a quick
# rebuild, pull kirigami, icons, etc
if [ "$SUBSURFACE_MOBILE" = "ON" ] && [ "$QUICK" = "" ] ; then
	pushd "$SUBSURFACE_SOURCE"
	bash ./scripts/mobilecomponents.sh
	popd
fi

if [ ! -z "$SUBSURFACE_MOBILE" ] ; then
	mkdir -p subsurface-mobile-build-"$ARCH"
	cd subsurface-mobile-build-"$ARCH"
	MOBILE_CMAKE=-DSUBSURFACE_TARGET_EXECUTABLE=MobileExecutable
	BUILD_NAME=Subsurface-mobile
else
	MOBILE_CMAKE=""
	mkdir -p subsurface-build-"$ARCH"
	cd subsurface-build-"$ARCH"
	BUILD_NAME=Subsurface
fi

if [ "$PLATFORM" = "Darwin" ] ; then
	FTDI=OFF
else
	FTDI=ON
fi

PKGCONF=$(which pkg-config)
cmake $MOBILE_CMAKE \
	-DPKG_CONFIG_EXECUTABLE="$PKGCONF" \
	-DQT_ANDROID_SDK_ROOT="$ANDROID_SDK_ROOT" \
	-DQT_ANDROID_NDK_ROOT="$ANDROID_NDK_ROOT" \
	-DANDROID_TOOLCHAIN="gcc" \
	-DANDROID_PLATFORM="$ANDROID_PLATFORM" \
	-DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_ROOT"/build/cmake/android.toolchain.cmake \
	-DQT_ANDROID_CMAKE="$BUILDROOT"/qt-android-cmake/AddQtAndroidApk.cmake \
	-DANDROID_STL="gnustl_shared" \
	-DFORCE_LIBSSH=OFF \
	-DLIBDC_FROM_PKGCONFIG=ON \
	-DLIBGIT2_FROM_PKGCONFIG=ON \
	-DNO_PRINTING=ON \
	-DNO_USERMANUAL=ON \
	-DNO_DOCS=ON \
	-DFBSUPPORT=OFF \
	-DCMAKE_PREFIX_PATH:UNINITIALIZED="$QT5_ANDROID_CMAKE" \
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
	-DMAKE_TESTS=OFF \
	-DFTDISUPPORT=${FTDI} \
	-DANDROID_NATIVE_LIBSSL="$BUILDROOT/ndk-$ARCH/sysroot/usr/lib/libssl.so" \
	-DANDROID_NATIVE_LIBCRYPT="$BUILDROOT/ndk-$ARCH/sysroot/usr/lib/libcrypto.so" \
	-DCMAKE_MAKE_PROGRAM="make" \
	"$SUBSURFACE_SOURCE"

# set up the version number

rm -f ssrf-version.h
make version

if [ ! -z "$SUBSURFACE_MOBILE" ] ; then
	SUBSURFACE_MOBILE_VERSION=$(grep MOBILE_VERSION_STRING ssrf-version.h | awk '{ print $3 }' | tr -d \" )
	SUBSURFACE_MOBILE_VERSION="$SUBSURFACE_MOBILE_VERSION ($(grep CANONICAL_VERSION_STRING ssrf-version.h | awk '{ print $3 }' | tr -d \"))"

	rm -rf android-mobile
	cp -a "$SUBSURFACE_SOURCE/android-mobile" .
	sed -i -e "s/@SUBSURFACE_MOBILE_VERSION@/$SUBSURFACE_MOBILE_VERSION/;s/@BUILD_NR@/$BUILD_NR/" android-mobile/AndroidManifest.xml
else
	SUBSURFACE_VERSION=$(grep CANONICAL_VERSION_STRING ssrf-version.h | awk '{ print $3 }' | tr -d \")

	# android-mobile is hardcoded in CMakeLists.txt nowadays.
	rm -rf android-mobile
	cp -a "$SUBSURFACE_SOURCE/android" android-mobile
	sed -i -e "s/@SUBSURFACE_VERSION@/\"$SUBSURFACE_VERSION\"/;s/@BUILD_NR@/$BUILD_NR/" android-mobile/AndroidManifest.xml
fi

# now make the translations
make translations
mkdir -p assets/translations
cp -a translations/*.qm assets/translations

# now build Subsurface and use the rest of the command line arguments
make "$@"

echo "Done building $BUILD_NAME for Android"
