#!/bin/bash
#
# build a multi architecture package for Android
#
# this requires Qt5.14 or newer with matching NDK
#
# the packaging/android/android-build-setup.sh sets up an environment that works for this

set -eu

exec 1> >(tee ./android-build.log) 2>&1

# get the absolute path
pushd "$(dirname "$0")/../../"
export SUBSURFACE_SOURCE=$PWD
cd ..
export BUILDROOT=$PWD
popd

# Set build defaults
# is this a release or debug build
BUILD_TYPE=Debug

# and now we need a monotonic build number...
if [ ! -f ./buildnr.dat ] ; then
	BUILDNR=0
else
	BUILDNR=$(cat ./buildnr.dat)
fi
BUILDNR=$((BUILDNR+1))
echo "${BUILDNR}" > ./buildnr.dat

# Read build variables
source $SUBSURFACE_SOURCE/packaging/android/variables.sh

# this assumes that the Subsurface source directory is in the same
# directory hierarchy as the SDK and NDK
export ANDROID_NDK_ROOT="$SUBSURFACE_SOURCE/../$ANDROID_NDK"
export ANDROID_SDK_ROOT="$SUBSURFACE_SOURCE/.."
export JAVA_OPTS="-Dsun.jnu.encoding=UTF-8 -Dfile.encoding=UTF-8"

QUICK=""
ARCHITECTURES=""
BUILD_ABIS=""
versionOnly=""

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
			BUILDNR=$1
			shift
			;;
		-version)
			# only update the version info without rebuilding
			# this is useful when working with Xcode
			versionOnly="1"
			;;
		arm|arm64|x86|x86_64)
			ARCHITECTURES=$1
			shift
			;;
		-quick)
			QUICK="1"
			shift
			;;
		*)
			echo "Unknown argument $1"
			exit 1
			;;
	esac
done

mkdir -p "$BUILDROOT"/subsurface-mobile-build
pushd "$BUILDROOT"/subsurface-mobile-build

# set up the Subsurface versions by hand
GITVERSION=$(cd "$SUBSURFACE_SOURCE" ; git describe --abbrev=12)
CANONICALVERSION=$(echo "$GITVERSION" | sed -e 's/-g.*$// ; s/^v//' | sed -e 's/-/./')
MOBILEVERSION=$(grep MOBILE "$SUBSURFACE_SOURCE"/cmake/Modules/version.cmake | cut -d\" -f 2)
echo "#define GIT_VERSION_STRING \"$GITVERSION\"" > ssrf-version.h
echo "#define CANONICAL_VERSION_STRING \"$CANONICALVERSION\"" >> ssrf-version.h
echo "#define MOBILE_VERSION_STRING \"$MOBILEVERSION\"" >> ssrf-version.h
SUBSURFACE_MOBILE_VERSION="$MOBILEVERSION ($CANONICALVERSION)"
popd

if [ "$versionOnly" = "1" ] ; then
	exit 0
fi

# pick the Qt setup and show the configuration info
if [ -n "${QT5_ANDROID+X}" ] ; then
	echo "Using Qt5 in $QT5_ANDROID"
elif [ -d "$SUBSURFACE_SOURCE/../${LATEST_QT}" ] ; then
	export QT5_ANDROID=$SUBSURFACE_SOURCE/../${LATEST_QT}
else
	echo "Cannot find Qt 5.12 or newer under $SUBSURFACE_SOURCE/.."
	exit 1
fi

QMAKE=$QT5_ANDROID/android/bin/qmake
echo $QMAKE
$QMAKE -query

export TOOLCHAIN="$ANDROID_NDK_ROOT"/toolchains/llvm/prebuilt/linux-x86_64
PATH=$TOOLCHAIN/bin:$PATH
export ANDROID_NDK_HOME=$ANDROID_NDK_ROOT  # redundant, but that's what openssl wants

# make sure we have the font that we need for OnePlus phones due to https://bugreports.qt.io/browse/QTBUG-69494
if [ ! -f "$SUBSURFACE_SOURCE"/android-mobile/Roboto-Regular.ttf ] ; then
       cp "$ANDROID_SDK_ROOT"/platforms/"$ANDROID_PLATFORMS"/data/fonts/Roboto-Regular.ttf "$SUBSURFACE_SOURCE"/android-mobile || exit 1
fi

# next, make sure that the libdivecomputer sources are downloaded and
# ready for autoconfig
pushd "$SUBSURFACE_SOURCE"
if [ ! -d libdivecomputer/src ] ; then
	git submodule init
	git submodule update
fi
if [ ! -f libdivecomputer/configure ] ; then
	cd libdivecomputer
	autoreconf -i
fi
popd

# build default architectures, or the given one?
if [ "$ARCHITECTURES" = "" ] ; then
	ARCHITECTURES="armv7a aarch64"
fi

# it would of course be too easy to use these terms consistently, so let's not
# no where do we support x86 style Android builds - there aren't any relevant devices
for ARCH in $ARCHITECTURES ; do
	if [ "$ARCH" = "armv7a" ] ; then
		ANDROID_ABI="armeabi-v7a"
	else
		ANDROID_ABI="arm64-v8a"
	fi
	BUILD_ABIS="$BUILD_ABIS $ANDROID_ABI"
done

# if this isn't just a quick rebuild, pull kirigami, icons, etc, and finally build the Googlemaps plugin
if [ "$QUICK" = "" ] ; then
	pushd "$SUBSURFACE_SOURCE"
	bash -x ./scripts/mobilecomponents.sh
	popd

	# build google maps plugin
	# this is the easy one as it uses qmake which ensures things get built for all platforms, etc
	"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . googlemaps
	# but unfortunately, on Android (and apparently only on Android) the naming pattern for geoservice
	# plugins has changed
	pushd googlemaps
	sed -i 's/TARGET = qtgeoservices_googlemaps/TARGET = plugins_geoservices_qtgeoservices_googlemaps/' googlemaps.pro
	popd
	QT_PLUGINS_PATH=$($QMAKE -query QT_INSTALL_PLUGINS)
	GOOGLEMAPS_BIN=libplugins_geoservices_qtgeoservices_googlemaps_arm64-v8a.so
	if [ ! -e "$QT_PLUGINS_PATH"/geoservices/$GOOGLEMAPS_BIN ] || [ googlemaps/.git/HEAD -nt "$QT_PLUGINS_PATH"/geoservices/$GOOGLEMAPS_BIN ] ; then
	    mkdir -p googlemaps-build
	    pushd googlemaps-build
	    $QMAKE ANDROID_ABIS="$BUILD_ABIS" ../googlemaps/googlemaps.pro
	    make -j4
            make install
	    popd
	fi
fi

# autoconf based libraries are harder
for ARCH in $ARCHITECTURES ; do
	echo "START building libraries for $ARCH"
	echo "====================================="

	# it would of course be too easy to use these terms consistently, so let's not
	if [ "$ARCH" = "armv7a" ] ; then
		EABI="eabi"
		BINUTIL_ARCH="arm"
		OPENSSL_ARCH="arm"
		ANDROID_ABI="armeabi-v7a"
	else
		EABI=""
		BINUTIL_ARCH="aarch64"
		OPENSSL_ARCH="arm64"
		ANDROID_ABI="arm64-v8a"
	fi

	export TARGET=$ARCH-linux-android
	export AR=$TOOLCHAIN/bin/$BINUTIL_ARCH-linux-android$EABI-ar
	export AS=$TOOLCHAIN/bin/$BINUTIL_ARCH-linux-android$EABI-as
	export CC=$TOOLCHAIN/bin/$TARGET$EABI$ANDROID_PLATFORM_LEVEL-clang
	export CXX=$TOOLCHAIN/bin/$TARGET$EABI$ANDROID_PLATFORM_LEVEL-clang++
	export LD=$TOOLCHAIN/bin/$BINUTIL_ARCH-linux-android$EABI-ld
	export RANLIB=$TOOLCHAIN/bin/$BINUTIL_ARCH-linux-android$EABI-ranlib
	export STRIP=$TOOLCHAIN/bin/$BINUTIL_ARCH-linux-android$EABI-strip

	# set up an install root and create part of the directory structure so the openssl
	# manual install below doesn't fail
	export PREFIX="$BUILDROOT"/install-root-"$ANDROID_ABI"
	mkdir -p "$PREFIX"/include
	mkdir -p "$PREFIX"/lib/pkgconfig
	export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig

	if [ "$QUICK" = "" ] ; then
		"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . openssl
		if [ ! -e "$PKG_CONFIG_PATH/libssl.pc" ] ; then
			# openssl build fails with these set
			export SYSROOT=""
			export CFLAGS=""
			export CPPFLAGS=""
			export CXXFLAGS=""

			mkdir -p openssl-build-"$ARCH"
			cp -r openssl/* openssl-build-"$ARCH"
			pushd openssl-build-"$ARCH"
			perl -pi -e 's/-mandroid//g' Configure
			./Configure shared android-"$OPENSSL_ARCH" no-ssl2 no-ssl3 no-comp no-hw no-engine no-asm \
			--prefix="$PREFIX" -DOPENSSL_NO_UI_CONSOLE -DOPENSSL_NO_STDIO \
			-D__ANDROID_API__=$ANDROID_PLATFORM_LEVEL
			make depend
			# follow the suggestions here: https://doc.qt.io/qt-5/android-openssl-support.html
			make SHLIB_VERSION_NUMBER= SHLIB_EXT=_1_1.so build_libs

			cp -RL include/openssl $PREFIX/include/openssl
			cp libcrypto.a $PREFIX/lib
			cp libcrypto_1_1.so* $PREFIX/lib
			cp libssl.a $PREFIX/lib
			cp libssl_1_1.so* $PREFIX/lib
			cp *.pc $PKG_CONFIG_PATH

			popd
		fi

	fi

	# autoconf seems to get lost without this -- but openssl fails if these are set
	SYSROOT="$ANDROID_NDK_ROOT"/toolchains/llvm/prebuilt/linux-x86_64/sysroot
	CFLAGS="--sysroot=${SYSROOT} -fPIC"
	CPPFLAGS="--sysroot=${SYSROOT} -fPIC"
	CXXFLAGS="--sysroot=${SYSROOT} -fPIC"

	if [ "$QUICK" = "" ] ; then
		"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . sqlite
		if [ ! -e "$PKG_CONFIG_PATH/sqlite3.pc" ] ; then
			mkdir -p sqlite-build-"$ARCH"
			pushd sqlite-build-"$ARCH"
			../sqlite/configure --host="$TARGET" --prefix="$PREFIX" --enable-static --disable-shared
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
		if [ ! -e "$PKG_CONFIG_PATH/libxml-2.0.pc" ] ; then
			mkdir -p libxml2-build-"$ARCH"
			pushd libxml2-build-"$ARCH"
			../libxml2/configure --host="$TARGET" --prefix="$PREFIX" --without-python --without-iconv --enable-static --disable-shared
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
		if [ ! -e "$PKG_CONFIG_PATH/libxslt.pc" ] ; then
			mkdir -p libxslt-build-"$ARCH"
			pushd libxslt-build-"$ARCH"
			../libxslt/configure --host="$TARGET" --prefix="$PREFIX" --with-libxml-prefix="$PREFIX" --without-python --without-crypto --enable-static --disable-shared
			make
			make install
			popd
		fi

		"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . libzip
		if [ ! -e "$PKG_CONFIG_PATH/libzip.pc" ] ; then
			# libzip expects a predefined macro that isn't there for our compiler
			pushd libzip
			git reset --hard
			sed -i 's/SIZEOF_SIZE_T/__SIZEOF_SIZE_T__/g' lib/compat.h
			# also, don't deal with manuals and bzip2
			sed -i 's/ADD_SUBDIRECTORY(man)//' CMakeLists.txt
			sed -i 's/^FIND_PACKAGE(ZLIB/#&/' CMakeLists.txt
			popd
			mkdir -p libzip-build-"$ARCH"
			pushd libzip-build-"$ARCH"
			cmake \
				-DCMAKE_C_COMPILER="$CC" \
				-DCMAKE_LINKER="$CC" \
				-DCMAKE_INSTALL_PREFIX="$PREFIX" \
				-DCMAKE_INSTALL_LIBDIR="lib" \
				-DBUILD_SHARED_LIBS=OFF \
				-DCMAKE_DISABLE_FIND_PACKAGE_BZip2=TRUE \
				-DZLIB_VERSION_STRING=1.2.7 \
				-DZLIB_LIBRARY=z \
				../libzip/
			make
			make install
			popd
		fi

		"${SUBSURFACE_SOURCE}"/scripts/get-dep-lib.sh singleAndroid . libgit2
		if [ ! -e "$PKG_CONFIG_PATH/libgit2.pc" ] ; then
			# We don't want to find the HTTP_Parser package of the build host by mistake
			mkdir -p libgit2-build-"$ARCH"
			pushd libgit2-build-"$ARCH"
			cmake \
				-DCMAKE_C_COMPILER="$CC" \
				-DCMAKE_LINKER="$CC" \
				-DBUILD_CLAR=OFF -DBUILD_SHARED_LIBS=OFF \
				-DCMAKE_INSTALL_PREFIX="$PREFIX" \
				-DCURL=OFF \
				-DUSE_SSH=OFF \
				-DOPENSSL_SSL_LIBRARY="$PREFIX"/lib/libssl_1_1.so \
				-DOPENSSL_CRYPTO_LIBRARY="$PREFIX"/lib/libcrypto_1_1.so \
				-DOPENSSL_INCLUDE_DIR="$PREFIX"/include \
				-D_OPENSSL_VERSION="${OPENSSL_VERSION}" \
				-DCMAKE_DISABLE_FIND_PACKAGE_HTTP_Parser=TRUE \
				../libgit2/
			make
			make install
			# Patch away pkg-config dependency to zlib, its there, i promise
			perl -pi -e 's/^(Requires.private:.*)zlib(.*)$/$1 $2/' "$PKG_CONFIG_PATH"/libgit2.pc
			popd
		fi

	fi # QUICK

	# on Android I can't make an integrated build work, so let's build Kirigami separately?
	mkdir -p "$BUILDROOT"/kirigami-build-"$ARCH"
	pushd "$BUILDROOT"/kirigami-build-"$ARCH"
			cmake \
				-DANDROID="1" \
				-DANDROID_ABI="$ANDROID_ABI" \
				-DBUILD_SHARED_LIBS="OFF" \
				-DBUILD_EXAMPLES="OFF" \
				-DCMAKE_PREFIX_PATH=/android/5.15.1/android/lib/cmake \
				-DCMAKE_C_COMPILER="$CC" \
				-DCMAKE_LINKER="$CC" \
				-DCMAKE_INSTALL_PREFIX="$PREFIX" \
				-DECM_DIR="$SUBSURFACE_SOURCE"/mobile-widgets/3rdparty/ECM \
				"$SUBSURFACE_SOURCE"/mobile-widgets/3rdparty/kirigami
			make
			make install
	popd

	CURRENT_SHA=$(cd "$SUBSURFACE_SOURCE"/libdivecomputer ; git describe)
	PREVIOUS_SHA=$(cat "libdivecomputer-${ARCH}.SHA" 2>/dev/null || echo)
	if [ ! "$CURRENT_SHA" = "$PREVIOUS_SHA" ] || [ ! -e "$PKG_CONFIG_PATH/libdivecomputer.pc" ] ; then
		mkdir -p libdivecomputer-build-"$ARCH"
		pushd libdivecomputer-build-"$ARCH"
		"$SUBSURFACE_SOURCE"/libdivecomputer/configure --host="$TARGET" --prefix="$PREFIX" --enable-static --disable-shared --enable-examples=no
		make
		make install
		popd
		echo "$CURRENT_SHA" > "libdivecomputer-${ARCH}.SHA"
	fi
	echo "DONE building libraries for $ARCH"
	echo "====================================="
done # ARCH

# set up the final build
pushd "$BUILDROOT"/subsurface-mobile-build
rm -rf android-build
mkdir android-build
pushd "$SUBSURFACE_SOURCE"/android-mobile
tar c . | (cd "$BUILDROOT"/subsurface-mobile-build/android-build ; tar x)
popd

# call qmake to set up the build
echo "Run qmake to setup the Subsurface-mobile build for all architectures"
$QMAKE BUILD_NR="$BUILDNR" BUILD_VERSION_NAME="$SUBSURFACE_MOBILE_VERSION" ANDROID_ABIS="$BUILD_ABIS" "$SUBSURFACE_SOURCE"/Subsurface-mobile.pro

# if this isn't just a quick rebuild compile the translations
if [ "$QUICK" = "" ] ; then
	pushd "$SUBSURFACE_SOURCE"/translations
	SRCS=$(ls ./*.ts | grep -v source)
	popd
	pushd "$SUBSURFACE_SOURCE"/packaging/android
	mkdir -p translation-assets
	for src in $SRCS; do
		"$QT5_ANDROID"/android/bin/lrelease "$SUBSURFACE_SOURCE"/translations/"$src" -qm translation-assets/"${src/.ts/.qm}"
	done
	popd
fi

# now build the Subsurface aab
make aab

