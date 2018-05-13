#!/bin/bash
#
# show what you are doing and stop when things break
set -x
set -e

doVersion=$1
DEBUGRELEASE="Release"
DRCONFIG="release"
ARCHS="armv7 arm64 x86_64"
TARGET="iphoneos"
TARGET2="Device"
# deal with all the command line arguments
while [[ $# -gt 0 ]] ; do
	arg="$1"
	case $arg in
		-debug)
			# build for debugging
			DEBUGRELEASE="Debug"
			DRCONFIG="qml_debug"
			;;
		-simulator)
			# build for the simulator instead of for a device
			ARCHS="x86_64"
			TARGET="iphonesimulator"
			TARGET2="simulator"
			;;
		*)
			echo "Unknown command line argument $arg"
			;;
	esac
	shift
done

# set up easy to use variables with the important paths
TOP=$(pwd)
SUBSURFACE_SOURCE=${TOP}/../../../subsurface
IOS_QT=${TOP}/Qt
QT_VERSION=$(cd ${IOS_QT}; ls -d [1-9]* | awk -F. '{ printf("%02d.%02d.%02d\n", $1,$2,$3); }' | sort -n | tail -1 | sed -e 's/\.0/\./g;s/^0//')

if [ -z $QT_VERSION ] ; then
	echo "Couldn't determine Qt version; giving up"
	exit 1
fi

# set up the Subsurface versions by hand
GITVERSION=$(git describe --abbrev=12)
CANONICALVERSION=$(git describe --abbrev=12 | sed -e 's/-g.*$// ; s/^v//' | sed -e 's/-/./')
MOBILEVERSION=$(grep MOBILE ../../cmake/Modules/version.cmake | cut -d\" -f 2)
echo "#define GIT_VERSION_STRING \"$GITVERSION\"" > subsurface-mobile/ssrf-version.h
echo "#define CANONICAL_VERSION_STRING \"$CANONICALVERSION\"" >> subsurface-mobile/ssrf-version.h
echo "#define MOBILE_VERSION_STRING \"$MOBILEVERSION\"" >> subsurface-mobile/ssrf-version.h

BUNDLE=org.subsurface-divelog.subsurface-mobile
if [ "${IOS_BUNDLE_PRODUCT_IDENTIFIER}" != "" ] ; then
	BUNDLE=${IOS_BUNDLE_PRODUCT_IDENTIFIER}
fi

# create Info.plist with the correct versions
cat Info.plist.in | sed "s/@MOBILE_VERSION@/$MOBILEVERSION/;s/@CANONICAL_VERSION@/$CANONICALVERSION/;s/@PRODUCT_BUNDLE_IDENTIFIER@/$BUNDLE/" > Info.plist

if [ "$doVersion" = "version" ] ; then
	exit 0
fi

# Build Subsurface-mobile by default
SUBSURFACE_MOBILE=1

pushd ${SUBSURFACE_SOURCE}
bash scripts/mobilecomponents.sh
popd


# now build all the dependencies for the three relevant architectures (x86_64 is for the simulator)

for ARCH in $ARCHS; do

echo next building for $ARCH

	INSTALL_ROOT=$TOP/install-root-$ARCH
	mkdir -p $INSTALL_ROOT/lib $INSTALL_ROOT/bin $INSTALL_ROOT/include
	PKG_CONFIG_LIBDIR=$INSTALL_ROOT/lib/pkgconfig

	declare -x PKG_CONFIG_PATH=$PKG_CONFIG_LIBDIR
	declare -x PREFIX=$INSTALL_ROOT

	if [ "$ARCH" = "x86_64" ] ; then
		declare -x SDK_NAME="iphonesimulator"
		declare -x TOOLCHAIN_FILE="${TOP}/iPhoneSimulatorCMakeToolchain"
		declare -x IOS_PLATFORM=SIMULATOR64
		declare -x BUILDCHAIN=x86_64-apple-darwin
	else
		declare -x SDK_NAME="iphoneos"
		declare -x TOOLCHAIN_FILE="${TOP}/iPhoneDeviceCMakeToolchain"
		declare -x IOS_PLATFORM=OS
		declare -x BUILDCHAIN=arm-apple-darwin
	fi
	declare -x ARCH_NAME=$ARCH
	declare -x SDK=$SDK_NAME
	declare -x SDK_DIR=`xcrun --sdk $SDK_NAME --show-sdk-path`
	declare -x PLATFORM_DIR=`xcrun --sdk $SDK_NAME --show-sdk-platform-path`

	declare -x CC=`xcrun -sdk $SDK_NAME -find clang`
	declare -x CXX=`xcrun -sdk $SDK_NAME -find clang++`
	declare -x LD=`xcrun -sdk $SDK_NAME -find ld`
	declare -x CFLAGS="-arch $ARCH_NAME -isysroot $SDK_DIR -miphoneos-version-min=6.0 -I$SDK_DIR/usr/include -fembed-bitcode"
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

	target=$ARCH
	hosttarget=$ARCH

	../../scripts/get-dep-lib.sh ios .

	# libxslt have too old config.sub
	pushd libxslt
	autoreconf --install
	popd
	if [ ! -e $PKG_CONFIG_LIBDIR/libxslt.pc ] ; then
		mkdir -p libxslt-build-$ARCH_NAME
		pushd libxslt-build-$ARCH_NAME
		../libxslt/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --without-python --without-crypto --enable-static --disable-shared
		make
		make install
		popd
	fi

	if [ ! -e $PKG_CONFIG_LIBDIR/libzip.pc ] ; then
		mkdir -p libzip-build-$ARCH_NAME
		pushd libzip-build-$ARCH_NAME
		../libzip/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared
		make
		make install
		popd
	fi

	pushd libgit2
	# libgit2 with -Wall on iOS creates megabytes of warnings...
	sed -i.bak 's/ADD_C_FLAG_IF_SUPPORTED(-W/# ADD_C_FLAG_IF_SUPPORTED(-W/' CMakeLists.txt
	popd

	if [ ! -e $PKG_CONFIG_LIBDIR/libgit2.pc ] ; then
		mkdir -p libgit2-build-$ARCH
		pushd libgit2-build-$ARCH
		cmake ../libgit2 \
		    -G "Unix Makefiles" \
		    -DBUILD_SHARED_LIBS="OFF" \
		    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
			-DSHA1_TYPE=builtin \
			-DBUILD_CLAR=OFF \
			-DCMAKE_INSTALL_PREFIX=${PREFIX} \
			-DCMAKE_PREFIX_PATH=${PREFIX} \
			-DCURL=OFF \
			-DUSE_SSH=OFF \
			../libgit2/
		sed -i.bak 's/C_FLAGS = /C_FLAGS = -Wno-nullability-completeness -Wno-expansion-to-defined /' CMakeFiles/git2.dir/flags.make
		make
		make install
		# Patch away pkg-config dependency to zlib, its there, i promise
		perl -pi -e 's/^(Requires.private:.*)zlib(.*)$/$1 $2/' $PKG_CONFIG_LIBDIR/libgit2.pc
		popd
	fi

# build libdivecomputer
	if [ ! -d ../../libdivecomputer/src ] ; then
		pushd ../..
		git submodule init
		git submodule update --recursive
		popd
	fi
	if [ ! -f ../../libdivecomputer/configure ] ; then
		pushd ../../libdivecomputer
		autoreconf --install
		autoreconf --install
		popd
	fi
	if [ ! -f libdivecomputer-${ARCH}.SHA ] ; then
		echo "" > libdivecomputer-${ARCH}.SHA
	fi
	CURRENT_SHA=$(cd ../../libdivecomputer ; git describe)
	PREVIOUS_SHA=$(cat libdivecomputer-${ARCH}.SHA)
	if [ ! "$CURRENT_SHA" = "$PREVIOUS_SHA" ] ; then
		echo $CURRENT_SHA > libdivecomputer-${ARCH}.SHA
		mkdir -p libdivecomputer-build-$ARCH
		pushd libdivecomputer-build-$ARCH
		../../../libdivecomputer/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared --enable-examples=no --without-libusb --without-hidapi --enable-ble
		make
		make install
		popd
	fi

done

# build googlemaps
mkdir -p googlemaps-build
pushd googlemaps-build
${IOS_QT}/${QT_VERSION}/ios/bin/qmake ../googlemaps/googlemaps.pro CONFIG+=release
make
popd

# now combine the libraries into fat libraries
rm -rf install-root
cp -a install-root-x86_64 install-root
if [ "$TARGET" = "iphoneos" ] ; then
	pushd install-root/lib
	for LIB in $(find . -type f -name \*.a); do
		lipo ../../install-root-armv7/lib/$LIB ../../install-root-arm64/lib/$LIB ../../install-root-x86_64/lib/$LIB -create -output $LIB
	done
	popd
fi

pushd ${SUBSURFACE_SOURCE}/translations
SRCS=$(ls *.ts | grep -v source)
popd
pushd Subsurface-mobile
mkdir -p translations
for src in $SRCS; do
	${IOS_QT}/${QT_VERSION}/ios/bin/lrelease ${SUBSURFACE_SOURCE}/translations/$src -qm translations/${src/.ts/.qm}
done
popd

# in order to be able to use xcode without going through Qt Creator
# call qmake directly

mkdir -p build-Subsurface-mobile-Qt_$(echo ${QT_VERSION} | tr . _)_for_iOS-${DEBUGRELEASE}
cd build-Subsurface-mobile-Qt_$(echo ${QT_VERSION} | tr . _)_for_iOS-${DEBUGRELEASE}
${IOS_QT}/${QT_VERSION}/ios/bin/qmake ../Subsurface-mobile/Subsurface-mobile.pro \
	-spec macx-ios-clang CONFIG+=$TARGET CONFIG+=$TARGET2 CONFIG+=$DRCONFIG

make qmake_all
