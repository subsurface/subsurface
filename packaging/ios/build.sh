#!/bin/bash
#
# show what you are doing and stop when things break
set -x
set -e

DEBUGRELEASE="Release"
ARCHS="armv7 arm64 x86_64"
TARGET="iphoneos"
TARGET2="Device"
# deal with all the command line arguments
while [[ $# -gt 0 ]] ; do
	arg="$1"
	case $arg in
		-version)
			# only update the version info without rebuilding
			# this is useful when working with Xcode
			versionOnly="1"
			;;
		-debug)
			# build for debugging
			DEBUGRELEASE="Debug"
			;;
		-simulator)
			# build for the simulator instead of for a device
			ARCHS="x86_64"
			TARGET="iphonesimulator"
			TARGET2="simulator"
			;;
		-all)
			# build both debug and release for all devices
			DEBUGRELEASE="All"
			;;
		*)
			echo "Unknown command line argument $arg"
			;;
	esac
	shift
done

# set up easy to use variables with the important paths
pushd "$(dirname "$0")/../../"
export SUBSURFACE_SOURCE=$PWD
cd ..
export PARENT_DIR=$PWD
popd

IOS_QT=~/Qt
QT_VERSION=$(cd "$IOS_QT"; ls -d [1-9]* | awk -F. '{ printf("%02d.%02d.%02d\n", $1,$2,$3); }' | sort -n | tail -1 | sed -e 's/\.0/\./g;s/^0//')

if [ -z $QT_VERSION ] ; then
	echo "Couldn't determine Qt version; giving up"
	exit 1
fi

# set up the Subsurface versions by hand
GITVERSION=$(cd "$SUBSURFACE_SOURCE" ; git describe --abbrev=12)
CANONICALVERSION=$(echo $GITVERSION | sed -e 's/-g.*$// ; s/^v//' | sed -e 's/-/./')
MOBILEVERSION=$(grep MOBILE "$SUBSURFACE_SOURCE"/cmake/Modules/version.cmake | cut -d\" -f 2)
echo "#define GIT_VERSION_STRING \"$GITVERSION\"" > "$SUBSURFACE_SOURCE"/ssrf-version.h
echo "#define CANONICAL_VERSION_STRING \"$CANONICALVERSION\"" >> "$SUBSURFACE_SOURCE"/ssrf-version.h
echo "#define MOBILE_VERSION_STRING \"$MOBILEVERSION\"" >> "$SUBSURFACE_SOURCE"/ssrf-version.h

BUNDLE=org.subsurface-divelog.subsurface-mobile
if [ "${IOS_BUNDLE_PRODUCT_IDENTIFIER}" != "" ] ; then
	BUNDLE=${IOS_BUNDLE_PRODUCT_IDENTIFIER}
fi

pushd "$SUBSURFACE_SOURCE"/packaging/ios
# create Info.plist with the correct versions
cat Info.plist.in | sed "s/@MOBILE_VERSION@/$MOBILEVERSION/;s/@CANONICAL_VERSION@/$CANONICALVERSION/;s/@PRODUCT_BUNDLE_IDENTIFIER@/$BUNDLE/" > Info.plist

popd
if [ "$versionOnly" = "1" ] ; then
	exit 0
fi

# Build Subsurface-mobile by default
SUBSURFACE_MOBILE=1

pushd "$SUBSURFACE_SOURCE"
bash scripts/mobilecomponents.sh
popd


# now build all the dependencies for the three relevant architectures (x86_64 is for the simulator)

# get all 3rd part libraries
"$SUBSURFACE_SOURCE"/scripts/get-dep-lib.sh ios "$PARENT_DIR"

for ARCH in $ARCHS; do

	echo next building for $ARCH

	INSTALL_ROOT=$PARENT_DIR/install-root/ios/$ARCH
	mkdir -p "$INSTALL_ROOT"/lib "$INSTALL_ROOT"/bin "$INSTALL_ROOT"/include
	PKG_CONFIG_LIBDIR="$INSTALL_ROOT"/lib/pkgconfig

	declare -x PKG_CONFIG_PATH=$PKG_CONFIG_LIBDIR
	declare -x PREFIX=$INSTALL_ROOT

	if [ "$ARCH" = "x86_64" ] ; then
		declare -x SDK_NAME="iphonesimulator"
		declare -x TOOLCHAIN_FILE="$SUBSURFACE_SOURCE/packaging/ios/iPhoneSimulatorCMakeToolchain"
		declare -x IOS_PLATFORM=SIMULATOR64
		declare -x BUILDCHAIN=x86_64-apple-darwin
	else
		declare -x SDK_NAME="iphoneos"
		declare -x TOOLCHAIN_FILE="$SUBSURFACE_SOURCE/packaging/ios/iPhoneDeviceCMakeToolchain"
		declare -x IOS_PLATFORM=OS
		declare -x BUILDCHAIN=arm-apple-darwin
	fi
	declare -x ARCH_NAME=$ARCH
	declare -x SDK=$SDK_NAME
	declare -x SDK_DIR=$(xcrun --sdk $SDK_NAME --show-sdk-path)
	declare -x PLATFORM_DIR=$(xcrun --sdk $SDK_NAME --show-sdk-platform-path)

	declare -x CC=$(xcrun -sdk $SDK_NAME -find clang)
	declare -x CXX=$(xcrun -sdk $SDK_NAME -find clang++)
	declare -x LD=$(xcrun -sdk $SDK_NAME -find ld)
	declare -x CFLAGS="-arch $ARCH_NAME -isysroot $SDK_DIR -miphoneos-version-min=10.0 -I$SDK_DIR/usr/include -fembed-bitcode"
	declare -x CXXFLAGS="$CFLAGS"
	declare -x LDFLAGS="$CFLAGS -lsqlite3 -lpthread -lc++ -L$SDK_DIR/usr/lib -fembed-bitcode"

	# openssl build stuff.
	export DEVELOPER=$(xcode-select --print-path)\
	export IPHONEOS_SDK_VERSION=$(xcrun --sdk iphoneos --show-sdk-version)
	export IPHONEOS_DEPLOYMENT_VERSION="6.0"
	export IPHONEOS_PLATFORM=$(xcrun --sdk iphoneos --show-sdk-platform-path)
	export IPHONEOS_SDK=$(xcrun --sdk iphoneos --show-sdk-path)
	export IPHONESIMULATOR_PLATFORM=$(xcrun --sdk iphonesimulator --show-sdk-platform-path)
	export IPHONESIMULATOR_SDK=$(xcrun --sdk iphonesimulator --show-sdk-path)
	export OSX_SDK_VERSION=$(xcrun --sdk macosx --show-sdk-version)
	export OSX_DEPLOYMENT_VERSION="10.8"
	export OSX_PLATFORM=$(xcrun --sdk macosx --show-sdk-platform-path)
	export OSX_SDK=$(xcrun --sdk macosx --show-sdk-path)

	# build libxml2 and libxslt
	if [ ! -e "$PKG_CONFIG_LIBDIR"/libxml-2.0.pc ] ; then
		if [ ! -e "$PARENT_DIR"/libxml2/configure ] ; then
			pushd "$PARENT_DIR"/libxml2
			autoreconf --install
			popd
		fi
		mkdir -p "$PARENT_DIR"/libxml2-build-"$ARCH"
		pushd "$PARENT_DIR"/libxml2-build-"$ARCH"
		"$PARENT_DIR"/libxml2/configure --host=${BUILDCHAIN} --prefix="$PREFIX" --without-lzma --without-python --without-iconv --enable-static --disable-shared
		perl -pi -e 's/runtest\$\(EXEEXT\)//' Makefile
		perl -pi -e 's/testrecurse\$\(EXEEXT\)//' Makefile
		make
		make install
		popd
	fi

	# the config.sub in libxslt is too old
	pushd "$PARENT_DIR"/libxslt
	autoreconf --install
	popd
	if [ ! -e "$PKG_CONFIG_LIBDIR"/libxslt.pc ] ; then
		mkdir -p "$PARENT_DIR"/libxslt-build-$ARCH
		pushd "$PARENT_DIR"/libxslt-build-$ARCH
		"$PARENT_DIR"/libxslt/configure --host=$BUILDCHAIN --prefix="$PREFIX" --with-libxml-include-prefix="$INSTALL_ROOT"/include/libxml2 --without-python --without-crypto --enable-static --disable-shared
		make
		make install
		popd
	fi

	if [ ! -e "$PKG_CONFIG_LIBDIR"/libzip.pc ] ; then
		pushd "$PARENT_DIR"/libzip
		# don't waste time on building command line tools, examples, manual, and regression tests - and don't build the BZIP2 support we don't need
		sed -i.bak 's/ADD_SUBDIRECTORY(src)//;s/ADD_SUBDIRECTORY(examples)//;s/ADD_SUBDIRECTORY(man)//;s/ADD_SUBDIRECTORY(regress)//' CMakeLists.txt
		mkdir -p "$PARENT_DIR"/libzip-build-$ARCH
		pushd "$PARENT_DIR"/libzip-build-$ARCH
		cmake -DBUILD_SHARED_LIBS="OFF" \
			-DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
			-DCMAKE_INSTALL_PREFIX="$PREFIX" \
			-DCMAKE_PREFIX_PATH="$PREFIX" \
			-DCMAKE_DISABLE_FIND_PACKAGE_BZip2=TRUE \
			-DENABLE_OPENSSL=FALSE \
			-DENABLE_GNUTLS=FALSE \
			"$PARENT_DIR"/libzip
		# quiet the super noise warnings
		sed -i.bak 's/C_FLAGS = /C_FLAGS = -Wno-nullability-completeness -Wno-expansion-to-defined /' lib/CMakeFiles/zip.dir/flags.make
		make
		make install
		popd
		mv CMakeLists.txt.bak CMakeLists.txt
		popd
	fi

	pushd "$PARENT_DIR"/libgit2
	# libgit2 with -Wall on iOS creates megabytes of warnings...
	sed -i.bak 's/ADD_C_FLAG_IF_SUPPORTED(-W/# ADD_C_FLAG_IF_SUPPORTED(-W/' CMakeLists.txt
	popd

	if [ ! -e $PKG_CONFIG_LIBDIR/libgit2.pc ] ; then
		mkdir -p "$PARENT_DIR"/libgit2-build-$ARCH
		pushd "$PARENT_DIR"/libgit2-build-$ARCH
		cmake "$PARENT_DIR"/libgit2 \
		    -G "Unix Makefiles" \
		    -DBUILD_SHARED_LIBS="OFF" \
		    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
			-DSHA1_TYPE=builtin \
			-DBUILD_CLAR=OFF \
			-DCMAKE_INSTALL_PREFIX="$PREFIX" \
			-DCMAKE_PREFIX_PATH="$PREFIX" \
			-DCURL=OFF \
			-DUSE_SSH=OFF \
			"$PARENT_DIR"/libgit2/
		sed -i.bak 's/C_FLAGS = /C_FLAGS = -Wno-nullability-completeness -Wno-expansion-to-defined /' src/CMakeFiles/git2.dir/flags.make
		make
		make install
		# Patch away pkg-config dependency to zlib, its there, i promise
		perl -pi -e 's/^(Requires.private:.*)zlib(.*)$/$1 $2/' $PKG_CONFIG_LIBDIR/libgit2.pc
		popd
	fi

# build libdivecomputer
	if [ ! -d "$SUBSURFACE_SOURCE"/libdivecomputer/src ] ; then
		pushd "$SUBSURFACE_SOURCE"
		git submodule init
		git submodule update --recursive
		popd
	fi
	if [ ! -f "$SUBSURFACE_SOURCE"/libdivecomputer/configure ] ; then
		pushd "$SUBSURFACE_SOURCE"/libdivecomputer
		autoreconf --install
		autoreconf --install
		popd
	fi
	mkdir -p "$PARENT_DIR"/libdivecomputer-build-$ARCH
	if [ ! -f "$PARENT_DIR"/libdivecomputer-build-$ARCH/git.SHA ] ; then
		echo "" > "$PARENT_DIR"/libdivecomputer-build-$ARCH/git.SHA
	fi
	CURRENT_SHA=$(cd "$SUBSURFACE_SOURCE"/libdivecomputer ; git describe)
	PREVIOUS_SHA=$(cat "$PARENT_DIR"/libdivecomputer-build-$ARCH/git.SHA)
	if [ ! "$CURRENT_SHA" = "$PREVIOUS_SHA" ] ; then
		echo $CURRENT_SHA > "$PARENT_DIR"/libdivecomputer-build-$ARCH/git.SHA
		pushd "$PARENT_DIR"/libdivecomputer-build-$ARCH
		${SUBSURFACE_SOURCE}/libdivecomputer/configure --host=${BUILDCHAIN} --prefix="$PREFIX" --enable-static --disable-shared --enable-examples=no --without-libusb --without-hidapi --enable-ble
		make
		make install
		popd
	fi

done

# build googlemaps
mkdir -p "$PARENT_DIR"/googlemaps-build
pushd "$PARENT_DIR"/googlemaps-build
"$IOS_QT"/"$QT_VERSION"/ios/bin/qmake "$PARENT_DIR"/googlemaps/googlemaps.pro CONFIG+=release
make
if [ "$DEBUGRELEASE" != "Release" ] ; then
	"$IOS_QT"/"$QT_VERSION"/ios/bin/qmake "$PARENT_DIR"/googlemaps/googlemaps.pro CONFIG+=debug
	make clean
	make
fi
popd

# build Kirigami
mkdir -p "$PARENT_DIR"/kirigami-build
pushd "$PARENT_DIR"/kirigami-build
"$IOS_QT"/"$QT_VERSION"/ios/bin/qmake "$SUBSURFACE_SOURCE"/mobile-widgets/3rdparty/kirigami/kirigami.pro CONFIG+=release
make
#make install
if [ "$DEBUGRELEASE" != "Release" ] ; then
	"$IOS_QT"/"$QT_VERSION"/ios/bin/qmake "$SUBSURFACE_SOURCE"/mobile-widgets/3rdparty/kirigami/kirigami.pro CONFIG+=debug
	make clean
	make
	#make install
fi
# since the install prefix for qmake is rather weirdly implemented, let's copy things by hand into the multiarch destination
mkdir -p "$INSTALL_ROOT"/../lib/qml/
cp -a org "$INSTALL_ROOT"/../lib/qml/
popd

# now combine the libraries into fat libraries
ARCH_ROOT=$PARENT_DIR/install-root/ios
cp -a "$ARCH_ROOT"/x86_64/* "$ARCH_ROOT"
if [ "$TARGET" = "iphoneos" ] ; then
	pushd "$ARCH_ROOT"/lib
	for LIB in $(find . -type f -name \*.a); do
		# libkirigamiplugin is already a fat library
		if grep -v -q "kirigami" <<< "$LIB" ; then
			lipo "$ARCH_ROOT"/armv7/lib/"$LIB" "$ARCH_ROOT"/arm64/lib/"$LIB" "$ARCH_ROOT"/x86_64/lib/"$LIB" -create -output "$LIB"
		fi
	done
	popd
fi

pushd "$SUBSURFACE_SOURCE"/translations
SRCS=$(ls ./*.ts | grep -v source)
popd
mkdir -p "$SUBSURFACE_SOURCE"/packaging/ios/build-ios/translations
for src in $SRCS; do
	"$IOS_QT"/"$QT_VERSION"/ios/bin/lrelease "$SUBSURFACE_SOURCE"/translations/"$src" -qm "$SUBSURFACE_SOURCE"/packaging/ios/build-ios/translations/"${src/.ts/.qm}"
done

# in order to be able to use xcode without going through Qt Creator
# call qmake directly

if [ "$DEBUGRELEASE" = "All" ] ; then
	BUILD_LOOP="Debug Release"
else
	BUILD_LOOP=$DEBUGRELEASE
fi
for BUILD_NOW in $BUILD_LOOP; do
	echo "Building for $BUILD_NOW"
	if [ "$BUILD_NOW" == "Debug" ] ; then
		DRCONFIG="qml_debug"
	else
		DRCONFIG="release"
	fi
	cd "$PARENT_DIR"
	BUILDX=build-Subsurface-mobile-Qt_$(echo "$QT_VERSION" | tr .  _)_for_iOS-"$BUILD_NOW"
	mkdir -p "$BUILDX"
	pushd "$BUILDX"
	rm -f ssrf-version.h
	ln -s "$SUBSURFACE_SOURCE"/ssrf-version.h .
	"$IOS_QT"/"$QT_VERSION"/ios/bin/qmake "$SUBSURFACE_SOURCE"/Subsurface-mobile.pro \
		-spec macx-ios-clang CONFIG+=$TARGET CONFIG+=$TARGET2 CONFIG+=$DRCONFIG

	make 
	popd
done
