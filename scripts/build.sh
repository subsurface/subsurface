#!/bin/bash
#
# this should be run from the src directory which contains the subsurface
# directory; the layout should look like this:
# .../src/subsurface
#
# the script will build Subsurface and libdivecomputer (plus some other
# dependencies if requestsed) from source.
#
# it installs the libraries and subsurface in the install-root subdirectory
# of the current directory (except on Mac where the Subsurface.app ends up
# in subsurface/build)
#
# by default it puts the build folders in
# ./subsurface/libdivecomputer/build (libdivecomputer build)
# ./subsurface/build                  (desktop build)
# ./subsurface/build-mobile           (mobile build)
# ./subsurface/build-downloader       (headless downloader build)
#
# there is basic support for building from a shared directory, e.g., with
# one subsurface source tree on a host computer, accessed from multiple
# VMs as well as the host to build without stepping on each other - the
# one exception is running autotools for libdiveconputer which has to
# happen in the shared libdivecomputer folder
# one way to achieve this is to have ./subsurface be a symlink; in that
# case the build directories are libdivecomputer/build, build, build-mobile
# alternatively a build prefix can be explicitly given with -build-prefix
# that build prefix is directly pre-pended to the destinations mentioned
# above - if this is a directory, it needs to end with '/'

# don't keep going if we run into an error
set -e

# create a log file of the build

exec 1> >(tee build.log) 2>&1

SRC=$(pwd)

if [[ -L subsurface && -d subsurface ]] ; then
	# ./subsurface is a symbolic link to the source directory, so let's
	# set up a prefix that puts the build directories in the current directory
	# but this can be overwritten via the command line
	BUILD_PREFIX="$SRC/"
fi

PLATFORM=$(uname)

BTSUPPORT="ON"
DEBUGRELEASE="Debug"
SRC_DIR="subsurface"
ARCHS=""

# deal with all the command line arguments
while [[ $# -gt 0 ]] ; do
	arg="$1"
	case $arg in
		-no-bt)
			# force Bluetooth support off
			BTSUPPORT="OFF"
			;;
		-quick)
			# only build libdivecomputer and Subsurface - this assumes that all other dependencies don't need rebuilding
			QUICK="1"
			;;
		-src-dir)
			# instead of using "subsurface" as source directory, use src/<srcdir>
			# this is convinient when using "git worktree" to have multiple branches
			# checked out on the computer.
			# remark <srcdir> must be in src (in parallel to subsurface), in order to
			# use the same 3rd party set.
			shift
			SRC_DIR="$1"
			;;
		-build-deps-only)
			# in order to build the dependencies on Mac for release builds (to deal with the macosx-version-min for those)
			# call this script with -build-deps
			BUILD_DEPS_ONLY="1"
			;;
		-make-package)
			# use this script to build dependencies and set things up with default values and then run make-mpackage.sh
			# to finish the build
			MAKE_PACKAGE="1"
			;;
		-fat-build)
			# build a fat binary for macOS
			# ignored on other platforms
			# this implies a Qt6 build (as m1 isn't supported in Qt5)
			ARCHS="arm64 x86_64"
			BUILD_WITH_QT6="1"
			;;
		-build-with-homebrew)
			# use libraries from Homebrew, don't try to create self-contained app
			BUILD_WITH_HOMEBREW="1"
			;;
		-build-prefix)
			# instead of building in build & build-mobile in the current directory, build in <buildprefix>build
			# and <buildprefix>build-mobile; notice that there's no slash between the prefix and the two directory
			# names, so if the prefix is supposed to be a path, add the slash at the end of it, or do funky things
			# where build/build-mobile get appended to partial path name
			shift
			BUILD_PREFIX="$1"
			;;
		-build-with-webkit)
			# unless you build Qt from source (or at least webkit from source, you won't have webkit installed
			# -build-with-webkit tells the script that in fact we can assume that webkit is present (it usually
			# is still available on Linux distros)
			BUILD_WITH_WEBKIT="1"
			;;
		-build-with-qt6)
			# Qt6 is not enabled by default as there are a few issues still with the port
			# - by default the Qt6 packages don't include QtLocation, so no maps (see below)
			# - WebKit doesn't work with Qt6, so no printing or in-app user manual
			# - there are a few other random bugs that we still find here and there
			# So by default we only try to build against Qt5. This overwrites that
			BUILD_WITH_QT6="1"
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
		-downloader)
			# we are building Subsurface-downloader
			BUILD_DOWNLOADER="1"
			;;
		-both)
			# we are building Subsurface and Subsurface-mobile
			BUILD_MOBILE="1"
			BUILD_DESKTOP="1"
			;;
		-all)
			# we are building Subsurface, Subsurface-mobile, and Subsurface-downloader
			BUILD_MOBILE="1"
			BUILD_DESKTOP="1"
			BUILD_DOWNLOADER="1"
			;;
		-ftdi)
			# make sure we include the user space FTDI drivers
			FTDI="1"
			;;
		-build-docs)
			# build the user manual
			BUILD_DOCS="1"
			;;
		-build-tests)
			# build the tests
			BUILD_TESTS="1"
			;;
		-install-docs)
			# include the user manual files in the packages
			INSTALL_DOCS="1"
			;;
		-create-appdir)
			# we are building an AppImage as by product
			CREATE_APPDIR="1"
			;;
		-release)
			# don't build Debug binaries
			DEBUGRELEASE="Release"
			;;
		*)
			echo "Unknown command line argument $arg"
			echo "Usage: build.sh [-all] [-both] [-build-deps-only] [-build-with-homebrew] [-build-prefix <PREFIX>] [-build-with-qt6] [-build-with-webkit] [-create-appdir] [-desktop] [-downloader] [-fat-build] [-ftdi] [-mobile] [-no-bt] [-make-package] [-quick] [-release] [-build-docs] [-build-tests] [-install-docs] [-src-dir <SUBSURFACE directory>] "
			exit 1
			;;
	esac
	shift
done

# Use all cores, unless user set their own MAKEFLAGS
if [[ -z "${MAKEFLAGS+x}" ]]; then
	if [[ ${PLATFORM} == "Linux" ]]; then
		NUM_CORES="$(nproc)"
	elif [[ ${PLATFORM} == "Darwin" ]]; then
		NUM_CORES="$(sysctl -n hw.logicalcpu)"
	else
		NUM_CORES="4"
	fi
	echo "Using ${NUM_CORES} cores for the build"
	export MAKEFLAGS="-j${NUM_CORES}"
else
	echo "Using user defined MAKEFLAGS=${MAKEFLAGS}"
fi

# recreate the old default behavior - no flag set implies build desktop
if [ "$BUILD_MOBILE$BUILD_DOWNLOADER" = "" ] ; then
	BUILD_DESKTOP="1"
fi

if [ "$BUILD_DEPS_ONLY" = "1" ] && [ "$QUICK" = "1" ] ; then
	echo "Conflicting options; cannot request combine -build-deps-only and -quick"
	exit 1;
fi

# normally this script builds the desktop version in subsurface/build
# the user can explicitly pick the builds requested
# for historic reasons, -both builds mobile and desktop, -all builds the downloader as well

if [ "$BUILD_MOBILE" = "1" ] ; then
	echo "building Subsurface-mobile in ${SRC_DIR}/build-mobile"
	BUILDS+=( "MobileExecutable" )
	BUILDDIRS+=( "${BUILD_PREFIX}build-mobile" )
fi
if [ "$BUILD_DOWNLOADER" = "1" ] ; then
	echo "building Subsurface-downloader in ${SRC_DIR}/build-downloader"
	BUILDS+=( "DownloaderExecutable" )
	BUILDDIRS+=( "${BUILD_PREFIX}build-downloader" )
fi
if [ "$BUILD_DESKTOP" = "1" ] || [ "$BUILDS" = "" ] ; then
	# if no option is given, we build the desktop version
	echo "building Subsurface in ${SRC_DIR}/build"
	BUILDS+=( "DesktopExecutable" )
	BUILDDIRS+=( "${BUILD_PREFIX}build" )
fi

if [[ ! -d "${SRC_DIR}" ]] ; then
	echo "please start this script from the directory containing the Subsurface source directory"
	exit 1
fi

if [ -z "$BUILD_PREFIX" ] ; then
	INSTALL_ROOT=$SRC/install-root
else
	INSTALL_ROOT="$BUILD_PREFIX"install-root
fi
mkdir -p "$INSTALL_ROOT"
export INSTALL_ROOT DEBUGRELEASE

# make sure we find our own packages first (e.g., libgit2 only uses pkg_config to find libssh2)
export PKG_CONFIG_PATH=$INSTALL_ROOT/lib/pkgconfig:$PKG_CONFIG_PATH

# Verify that the Xcode Command Line Tools are installed
if [ "$PLATFORM" = Darwin ] ; then
	SDKROOT=$(xcrun --show-sdk-path)
	if [[ ! -d "${SDKROOT}" && ! -d "${SDKROOT}/usr/include" ]] ; then
		echo "Cannot find SDK (usually /Developer/SDKs or"
		echo "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs)"
		exit 1;
	fi
	# Apple tells us NOT to try to specify a specific SDK anymore and instead let its tools
	# do "the right thing" - especially don't set a sysroot (which way back when was required for this to work)
	# unforunately that means we need to somehow hard-code a deployment target, hoping the local tools
	# know how to build for that. Which seems... odd
	BASESDK="12.3"
	export BASESDK
	if [ "$ARCHS" != "" ] ; then
		# we do assume that the two architectures mentioned are x86_64 and arm64 .. that's kinda wrong
		MAC_CMAKE="-DCMAKE_OSX_DEPLOYMENT_TARGET=${BASESDK} -DCMAKE_OSX_ARCHITECTURES='x86_64;arm64' -DCMAKE_BUILD_TYPE=${DEBUGRELEASE} -DCMAKE_INSTALL_PREFIX=${INSTALL_ROOT} -DCMAKE_POLICY_VERSION_MINIMUM=3.16"
		MAC_OPTS="-mmacosx-version-min=${BASESDK} -arch arm64 -arch x86_64"
	else
		ARCHS=$(uname -m) # crazy, I know, but $(arch) results in the incorrect 'i386' on an x86_64 Mac
		MAC_CMAKE="-DCMAKE_OSX_DEPLOYMENT_TARGET=${BASESDK} -DCMAKE_OSX_ARCHITECTURES="$ARCHS" -DCMAKE_BUILD_TYPE={$DEBUGRELEASE} -DCMAKE_INSTALL_PREFIX=${INSTALL_ROOT} -DCMAKE_POLICY_VERSION_MINIMUM=3.16"
		MAC_OPTS="-mmacosx-version-min=${BASESDK}"
	fi
	# OpenSSL can't deal with multi arch build
	MAC_OPTS_OPENSSL="-mmacosx-version-min=${BASESDK}"
	echo "Using ${BASESDK} as the BASESDK under ${SDKROOT}"

	# if all we want is to build the dependencies, we are done with prep here
	if [[ "$BUILD_DEPS_ONLY" == "1" ]] ; then
		export ARCHS SRC SRC_DIR MAC_CMAKE MAC_OPTS MAC_OPTS_OPENSSL
		bash "./${SRC_DIR}/packaging/macosx/build-deps.sh"
		exit
	fi
fi

echo Building from "$SRC", installing in "$INSTALL_ROOT"

# find qmake
if [ -n "$CMAKE_PREFIX_PATH" ] ; then
	QMAKE=$CMAKE_PREFIX_PATH/../../bin/qmake
fi
if [[ -z $QMAKE  ||  ! -x $QMAKE ]] ; then
	[ -z $QMAKE ] && [ "$BUILD_WITH_QT6" = "1" ] && hash qmake6 > /dev/null 2> /dev/null && QMAKE=qmake6
	[ -z $QMAKE ] && [ "$BUILD_WITH_QT6" = "1" ] && hash qmake-qt6 > /dev/null 2> /dev/null && QMAKE=qmake-qt6
	[ -z $QMAKE ] && hash qmake > /dev/null 2> /dev/null && QMAKE=qmake
	[ -z $QMAKE ] && hash qmake-qt5 > /dev/null 2> /dev/null && QMAKE=qmake-qt5
	[ -z $QMAKE ] && hash qmake-qt6 > /dev/null 2> /dev/null && QMAKE=qmake-qt6
	[ -z $QMAKE ] && echo "cannot find qmake, qmake-qt5, or qmake-qt6" && exit 1
fi

# grab the Qt version
QT_VERSION=$($QMAKE -query QT_VERSION)

# it's not entirely clear why we only set this on macOS, but this appears to be what works
if [ "$PLATFORM" = Darwin ] ; then
	if [ -z "$CMAKE_PREFIX_PATH" ] ; then
		# we already found qmake and can get the right path information from that
		libdir=$($QMAKE -query QT_INSTALL_LIBS)
		if [ $? -eq 0 ]; then
			export CMAKE_PREFIX_PATH=$libdir/cmake
		else
			echo "something is broken with the Qt install"
			exit 1
		fi
	fi
fi


# on Debian and Ubuntu based systems, the private QtLocation and
# QtPositioning headers aren't bundled. Download them if necessary.
if [ "$PLATFORM" = Linux ] && [[ $QT_VERSION == 5* ]] ; then
	QT_HEADERS_PATH=$($QMAKE -query QT_INSTALL_HEADERS)

	if [ ! -d "$QT_HEADERS_PATH/QtLocation/$QT_VERSION/QtLocation/private" ] &&
           [ ! -d "$INSTALL_ROOT"/include/QtLocation/private ] ; then
		echo "Missing private Qt headers for $QT_VERSION; downloading them..."

		QTLOC_GIT=./qtlocation_git
		QTLOC_PRIVATE=$INSTALL_ROOT/include/QtLocation/private
		QTPOS_PRIVATE=$INSTALL_ROOT/include/QtPositioning/private

		rm -rf $QTLOC_GIT > /dev/null 2>&1
		rm -rf "$INSTALL_ROOT"/include/QtLocation > /dev/null 2>&1
		rm -rf "$INSTALL_ROOT"/include/QtPositioning > /dev/null 2>&1

		git clone --branch "v$QT_VERSION" https://github.com/qt/qtlocation.git --depth=1 $QTLOC_GIT ||
			git clone --branch "v$QT_VERSION-lts-lgpl" https://github.com/qt/qtlocation.git --depth=1 $QTLOC_GIT

		mkdir -p "$QTLOC_PRIVATE"
		cd $QTLOC_GIT/src/location
		find . -name '*_p.h' -print0 | xargs -0 cp -t "$QTLOC_PRIVATE"
		cd "$SRC"

		mkdir -p "$QTPOS_PRIVATE"
		cd $QTLOC_GIT/src/positioning
		find . -name '*_p.h' -print0 | xargs -0 cp -t "$QTPOS_PRIVATE"
		cd "$SRC"

		echo "* cleanup..."
		rm -rf $QTLOC_GIT > /dev/null 2>&1
	fi
fi

# set up the right file name extensions
if [ "$PLATFORM" = Darwin ] ; then
	SH_LIB_EXT=dylib
	if [ ! "$BUILD_DEPS_ONLY" == "1" ] ; then
		pkg-config --exists libgit2 && LIBGIT=$(pkg-config --modversion libgit2) && LIBGITMAJ=$(echo $LIBGIT | cut -d. -f1) && LIBGIT=$(echo $LIBGIT | cut -d. -f2)
		if [[ "$LIBGITMAJ" -gt "0" || "$LIBGIT" -gt "25" ]] ; then
			LIBGIT2_FROM_PKGCONFIG="-DLIBGIT2_FROM_PKGCONFIG=ON"
		fi
	fi
else
	SH_LIB_EXT=so

	LIBGIT_ARGS="-DLIBGIT2_DYNAMIC=ON"
	# check if we need to build libgit2 (and do so if necessary)
	# first check pkgconfig (that will capture our own local build if
	# this script has been run before)
	if pkg-config --exists libgit2 ; then
		LIBGIT=$(pkg-config --modversion libgit2)
		LIBGITMAJ=$(echo $LIBGIT | cut -d. -f1)
		LIBGIT=$(echo $LIBGIT | cut -d. -f2)
		if [[ "$LIBGITMAJ" -gt "0" || "$LIBGIT" -gt "25" ]] ; then
			LIBGIT2_FROM_PKGCONFIG="-DLIBGIT2_FROM_PKGCONFIG=ON"
		fi
	fi
	if [[ "$LIBGITMAJ" -lt "1" && "$LIBGIT" -lt "26" ]] ; then
		# maybe there's a system version that's new enough?
		# Ugh that's uggly - read the ultimate filename, split at the last 'o' which gets us ".0.26.3" or ".1.0.0"
		# since that starts with a dot, the field numbers in the cut need to be one higher
		LDCONFIG=$(PATH=/sbin:/usr/sbin:$PATH which ldconfig)
		if [ ! -z "$LDCONFIG" ] ; then
			LIBGIT=$(realpath $("$LDCONFIG" -p | grep libgit2\\.so\\. | cut -d\  -f4) | awk -Fo '{ print $NF }')
			LIBGITMAJ=$(echo $LIBGIT | cut -d. -f2)
			LIBGIT=$(echo $LIBGIT | cut -d. -f3)
		fi
	fi
fi

cd "$SRC"

if [[ $PLATFORM != Darwin && "$LIBGITMAJ" -lt "1" && "$LIBGIT" -lt "26" ]] ; then
	LIBGIT_ARGS="-DLIBGIT2_INCLUDE_DIR=$INSTALL_ROOT/include -DLIBGIT2_LIBRARIES=$INSTALL_ROOT/lib/libgit2.$SH_LIB_EXT"

	./${SRC_DIR}/scripts/get-dep-lib.sh single . libgit2
	pushd libgit2
	mkdir -p build
	cd build
	cmake $MAC_CMAKE -DBUILD_CLAR=OFF ..
	make
	make install
	popd
fi

# build libdivecomputer

cd ${SRC_DIR}

# Remove a stale generated ssrf-version.h file (if any)
rm -f ssrf-version.h

if [ ! -d libdivecomputer/src ] ; then
	git submodule init
	git submodule update --recursive
fi

mkdir -p "${BUILD_PREFIX}libdivecomputer/build"
cd "${BUILD_PREFIX}libdivecomputer/build"

if [ ! -f "$SRC"/${SRC_DIR}/libdivecomputer/configure ] ; then
	# this is not a typo
	# in some scenarios it appears that autoreconf doesn't copy the
	# ltmain.sh file; running it twice, however, fixes that problem
	autoreconf --install "$SRC"/${SRC_DIR}/libdivecomputer
	autoreconf --install "$SRC"/${SRC_DIR}/libdivecomputer
fi

CFLAGS="$MAC_OPTS -I$INSTALL_ROOT/include $LIBDC_CFLAGS" "$SRC"/${SRC_DIR}/libdivecomputer/configure --prefix="$INSTALL_ROOT" --disable-examples

if [ "$PLATFORM" = Darwin ] ; then
	# remove some copmpiler options that aren't supported on Mac
	# otherwise the log gets very noisy
	for i in $(find . -name Makefile)
	do
		sed -i .bak 's/-Wrestrict//;s/-Wno-unused-but-set-variable//' "$i"
	done
	# it seems that on my Mac some of the configure tests for libdivecomputer
	# pass even though the feature tested for is actually missing
	# let's hack around that
	# touch config.status, recreate config.h and then disable HAVE_CLOCK_GETTIME
	# this seems to work so that the Makefile doesn't re-run the
	# configure process and overwrite all the changes we just made
	touch config.status
	make config.h
	grep CLOCK_GETTIME config.h
	sed -i .bak 's/^#define HAVE_CLOCK_GETTIME 1/#undef HAVE_CLOCK_GETTIME /' config.h
fi
make
make install
# make sure we know where the libdivecomputer.a was installed - sometimes it ends up in lib64, sometimes in lib
STATIC_LIBDC="$INSTALL_ROOT/$(grep ^libdir Makefile | cut -d/ -f2)/libdivecomputer.a"

cd "$SRC"

if [ "$QUICK" != "1" ] && [ "$BUILD_DESKTOP$BUILD_MOBILE" != "" ] ; then
	# build the googlemaps map plugin

	cd "$SRC"
	./${SRC_DIR}/scripts/get-dep-lib.sh single . googlemaps
	pushd googlemaps
	if [[ $QT_VERSION == 6* ]]; then
		# the latest version of googlemaps as of Nov 2025 builds out of the box with Qt 6.10
		# but no longer builds against Qt 5... so we have two branches now
		git checkout qt6-upstream
	fi
	mkdir -p build
	cd build
	$QMAKE "INCLUDEPATH=$INSTALL_ROOT/include" "CONFIG+=release" ../googlemaps.pro
	make
	make install
	popd
fi

if [[ "$QUICK" != "1" && "$BUILD_DESKTOP" == "1" && "$BUILD_WITH_QT6" == "1" ]] ; then
	# build the qlitehtml library
	cd "$SRC"
	./${SRC_DIR}/scripts/get-dep-lib.sh single . qlitehtml
	pushd qlitehtml
	git submodule init
	git submodule update

	# qlitehtml currently (2025-12-01) only allows in source tree builds
	cmake $MAC_CMAKE .
	make
	make install
	if [ "$PLATFORM" = Darwin ] ; then
		# now fix the @rpath entries in the .dylib
		install_name_tool -add_rpath "$(qmake -query QT_INSTALL_LIBS)" "$INSTALL_ROOT"/lib/libqlitehtml.1.dylib
	fi
	popd
fi

# finally, build Subsurface

set -x

for (( i=0 ; i < ${#BUILDS[@]} ; i++ )) ; do
	SUBSURFACE_EXECUTABLE=${BUILDS[$i]}
	BUILDDIR=${BUILDDIRS[$i]}
	echo "build $SUBSURFACE_EXECUTABLE in $BUILDDIR"

	if [ "$SUBSURFACE_EXECUTABLE" = "DesktopExecutable" ] && [ "$BUILD_WITH_WEBKIT" = "1" ]; then
		EXTRA_OPTS="-DNO_PRINTING=OFF"
	else
		EXTRA_OPTS="-DNO_PRINTING=ON"
	fi
	if [ "$FTDI" = "1" ] ; then
		EXTRA_OPTS="$EXTRA_OPTS -DFTDISUPPORT=ON"
	fi
	if [ "$BUILD_DOCS" = "1" ] ; then
		EXTRA_OPTS="$EXTRA_OPTS -DBUILD_DOCS=ON"
	fi
	if [ "$INSTALL_DOCS" = "1" ] ; then
		EXTRA_OPTS="$EXTRA_OPTS -DINSTALL_DOCS=ON"
	fi
	if [ "$BUILD_WITH_QT6" = "1" ] ; then
		EXTRA_OPTS="$EXTRA_OPTS -DBUILD_WITH_QT6=ON"
	fi
	if [ "$BUILD_TESTS" = "1" ] ; then
		EXTRA_OPTS="$EXTRA_OPTS -DBUILD_TESTS=ON"
	fi

	cd "$SRC"/${SRC_DIR}

	# pull the plasma-mobile components from upstream if building Subsurface-mobile
	if [ "$SUBSURFACE_EXECUTABLE" = "MobileExecutable" ] ; then
		bash ./scripts/mobilecomponents.sh
		EXTRA_OPTS="$EXTRA_OPTS -DECM_DIR=$SRC/$SRC_DIR/mobile-widgets/3rdparty/ECM"
	fi

	mkdir -p "$BUILDDIR"
	cd "$BUILDDIR"
	export CMAKE_PREFIX_PATH="$INSTALL_ROOT/lib/cmake;${CMAKE_PREFIX_PATH}"
	cmake -DCMAKE_BUILD_TYPE="$DEBUGRELEASE" \
		-DSUBSURFACE_TARGET_EXECUTABLE="$SUBSURFACE_EXECUTABLE" \
		"$LIBGIT_ARGS" \
		-DLIBDIVECOMPUTER_INCLUDE_DIR="$INSTALL_ROOT"/include \
		-DLIBDIVECOMPUTER_LIBRARIES="$STATIC_LIBDC" \
		-DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" \
		-DBTSUPPORT="$BTSUPPORT" \
		-DCMAKE_INSTALL_PREFIX="$INSTALL_ROOT" \
		$LIBGIT2_FROM_PKGCONFIG \
		-DFORCE_LIBSSH=OFF \
		$EXTRA_OPTS \
		"$SRC"/${SRC_DIR}

	if [ "$PLATFORM" = Darwin ] ; then
		rm -rf Subsurface.app
		rm -rf Subsurface-mobile.app

		if [ "$DEBUGRELEASE" = Debug ] ; then
			cp "$SRC"/${SRC_DIR}/entitlements-mac-dev.plist .
		fi
	fi

	if [[ "$MAKE_PACKAGE" = "1" && "$BUILDDIR" = "build" && "$PLATFORM" = "Darwin" ]] ; then
		# special case of building a distributable macOS package
		echo "finished initial cmake setup of Subsurface - next run the packaging script to build the DMG"
		bash -e -x ../packaging/macosx/make-package.sh | tee "$SRC"/mp.log 2>&1
		IMG=$(grep ^created: "$SRC"/mp.log | tail -1 | cut -b10-)
		echo "Created $IMG"
	else
		LIBRARY_PATH=$INSTALL_ROOT/lib make
		if [ "$BUILD_WITH_HOMEBREW" != "1" ]; then
			LIBRARY_PATH=$INSTALL_ROOT/lib make install
		fi

		if [ "$CREATE_APPDIR" = "1" ] ; then
			# if we create an AppImage this makes gives us a sane starting point
			cd "$SRC"
			mkdir -p ./appdir
			mkdir -p appdir/usr/share/metainfo
			mkdir -p appdir/usr/share/icons/hicolor/256x256/apps
			cp -r ./install-root/* ./appdir/usr
			cp ${SRC_DIR}/metainfo/subsurface.metainfo.xml appdir/usr/share/metainfo/
			cp ${SRC_DIR}/icons/subsurface-icon.png appdir/usr/share/icons/hicolor/256x256/apps/
		fi
	fi
done
