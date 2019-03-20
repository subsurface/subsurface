#!/bin/bash
#
# show what you are doing and stop when things break
set -x
set -e

doVersion=$1
DEBUGRELEASE="Release"
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
TOP=$(pwd)
SUBSURFACE_SOURCE=${TOP}/../../../subsurface
pushd ${SUBSURFACE_SOURCE}/..
SSRF_CLONE=$(pwd)
popd

# prepare build dir
mkdir -p build-ios

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
echo "#define GIT_VERSION_STRING \"$GITVERSION\"" > ssrf-version.h
echo "#define CANONICAL_VERSION_STRING \"$CANONICALVERSION\"" >> ssrf-version.h
echo "#define MOBILE_VERSION_STRING \"$MOBILEVERSION\"" >> ssrf-version.h

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

# get all 3rd part libraries
../../scripts/get-dep-lib.sh ios ${SSRF_CLONE}

for ARCH in $ARCHS; do

	echo next building for $ARCH

	INSTALL_ROOT=$TOP/build-ios/install-root-$ARCH
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

	# libxslt have too old config.sub
	pushd ${SSRF_CLONE}/libxslt
	autoreconf --install
	popd
	if [ ! -e $PKG_CONFIG_LIBDIR/libxslt.pc ] ; then
		mkdir -p build-ios/libxslt-build-$ARCH_NAME
		pushd build-ios/libxslt-build-$ARCH_NAME
		${SSRF_CLONE}/libxslt/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --without-python --without-crypto --enable-static --disable-shared
		make
		make install
		popd
	fi

	if [ ! -e $PKG_CONFIG_LIBDIR/libzip.pc ] ; then
		pushd ${SSRF_CLONE}/libzip
		# don't waste time on building command line tools, examples, manual, and regression tests - and don't build the BZIP2 support we don't need
		sed -i.bak 's/ADD_SUBDIRECTORY(src)//;s/ADD_SUBDIRECTORY(examples)//;s/ADD_SUBDIRECTORY(man)//;s/ADD_SUBDIRECTORY(regress)//' CMakeLists.txt
		mkdir -p build-ios/libzip-build-$ARCH_NAME
		pushd build-ios/libzip-build-$ARCH_NAME
		cmake -DBUILD_SHARED_LIBS="OFF" \
			-DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
			-DCMAKE_INSTALL_PREFIX=${PREFIX} \
			-DCMAKE_PREFIX_PATH=${PREFIX} \
			-DCMAKE_DISABLE_FIND_PACKAGE_BZip2=TRUE \
			${SSRF_CLONE}/libzip
		# quiet the super noise warnings
		sed -i.bak 's/C_FLAGS = /C_FLAGS = -Wno-nullability-completeness -Wno-expansion-to-defined /' lib/CMakeFiles/zip.dir/flags.make
		make
		make install
		popd
		mv CMakeLists.txt.bak CMakeLists.txt
		popd
	fi

	pushd ${SSRF_CLONE}/libgit2
	# libgit2 with -Wall on iOS creates megabytes of warnings...
	sed -i.bak 's/ADD_C_FLAG_IF_SUPPORTED(-W/# ADD_C_FLAG_IF_SUPPORTED(-W/' CMakeLists.txt
	popd

	if [ ! -e $PKG_CONFIG_LIBDIR/libgit2.pc ] ; then
		mkdir -p build-ios/libgit2-build-$ARCH
		pushd build-ios/libgit2-build-$ARCH
		cmake ${SSRF_CLONE}/libgit2 \
		    -G "Unix Makefiles" \
		    -DBUILD_SHARED_LIBS="OFF" \
		    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
			-DSHA1_TYPE=builtin \
			-DBUILD_CLAR=OFF \
			-DCMAKE_INSTALL_PREFIX=${PREFIX} \
			-DCMAKE_PREFIX_PATH=${PREFIX} \
			-DCURL=OFF \
			-DUSE_SSH=OFF \
			${SSRF_CLONE}/libgit2/
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
	if [ ! -f build-ios/libdivecomputer-${ARCH}.SHA ] ; then
		echo "" > build-ios/libdivecomputer-${ARCH}.SHA
	fi
	CURRENT_SHA=$(cd ../../libdivecomputer ; git describe)
	PREVIOUS_SHA=$(cat build-ios/libdivecomputer-${ARCH}.SHA)
	if [ ! "$CURRENT_SHA" = "$PREVIOUS_SHA" ] ; then
		echo $CURRENT_SHA > build-ios/libdivecomputer-${ARCH}.SHA
		mkdir -p build-ios/libdivecomputer-build-$ARCH
		pushd build-ios/libdivecomputer-build-$ARCH
		../../../../libdivecomputer/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared --enable-examples=no --without-libusb --without-hidapi --enable-ble
		make
		make install
		popd
	fi

done

# build googlemaps
mkdir -p build-ios/googlemaps-build
pushd build-ios/googlemaps-build
${IOS_QT}/${QT_VERSION}/ios/bin/qmake ${SSRF_CLONE}/googlemaps/googlemaps.pro CONFIG+=release
make
popd

# now combine the libraries into fat libraries
rm -rf install-root
cp -a build-ios/install-root-x86_64 install-root
if [ "$TARGET" = "iphoneos" ] ; then
	pushd install-root/lib
	for LIB in $(find . -type f -name \*.a); do
		lipo ../../build-ios/install-root-armv7/lib/$LIB ../../build-ios/install-root-arm64/lib/$LIB ../../build-ios/install-root-x86_64/lib/$LIB -create -output $LIB
	done
	popd
fi

pushd ${SUBSURFACE_SOURCE}/translations
SRCS=$(ls *.ts | grep -v source)
popd
mkdir -p build-ios/translations
for src in $SRCS; do
	${IOS_QT}/${QT_VERSION}/ios/bin/lrelease ${SUBSURFACE_SOURCE}/translations/$src -qm build-ios/translations/${src/.ts/.qm}
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
	BUILDX=build-Subsurface-mobile-Qt_$(echo ${QT_VERSION} | tr . _)_for_iOS-${BUILD_NOW}
	mkdir -p ${BUILDX}
	pushd ${BUILDX}
	${IOS_QT}/${QT_VERSION}/ios/bin/qmake ../Subsurface-mobile.pro \
		-spec macx-ios-clang CONFIG+=$TARGET CONFIG+=$TARGET2 CONFIG+=$DRCONFIG

	make qmake_all
	popd
done
