#!/bin/bash -eu

# run this in the top level folder you want to create Android binaries in
#
# it seems that with Qt5.7 (and later) and current cmake there is an odd bug where
# cmake fails reporting :No known features for CXX compiler "GNU". In that
# case simly comment out the "set(property(TARGET Qt5::Core PROPERTY...)"
# at line 101 of
# Qt/5.7/android_armv7/lib/cmake/Qt5Core/Qt5CoreConfigExtras.cmake
# or at line 95 of
# Qt/5.8/android_armv7/lib/cmake/Qt5Core/Qt5CoreConfigExtras.cmake
# or at line 105 of
# Qt/5.9/android_armv7/lib/cmake/Qt5Core/Qt5CoreConfigExtras.cmake
# (this script tries to do this automatically)

exec 1> >(tee ./build.log) 2>&1

USE_X=$(case $- in *x*) echo "-x" ;; esac)
QUICK=""
RELEASE=""

# deal with the command line arguments
while [[ $# -gt 0 ]] ; do
        arg="$1"
        case $arg in
                -prep-only)
			# only download the dependencies, don't build
			PREP_ONLY="1"
			;;
		-quick)
			# pass through to build.sh
			QUICK="-quick"
			;;
		release|Release)
			# pass through to build.sh
			RELEASE="release"
			;;
		*)
			echo "Unknown command line argument $arg"
			echo "Usage: $0 [-prep-only]"
			exit 1
			;;
	esac
	shift
done

# these are the current versions for Qt, Android SDK & NDK:
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"
source "$SCRIPTDIR"/variables.sh

PLATFORM=$(uname)

export SUBSURFACE_SOURCE="$SCRIPTDIR"/../..

if [ "$PLATFORM" = Linux ] ; then
	QT_BINARIES=qt-opensource-linux-x64-${LATEST_QT}.run
	NDK_BINARIES=${ANDROID_NDK}-linux-x86_64.zip
	SDK_TOOLS=sdk-tools-linux-${SDK_VERSION}.zip
else
	echo "only on Linux so far"
	exit 1
fi

# make sure we have the required commands installed
MISSING=
for i in git cmake autoconf libtool java wget unzip; do
	command -v $i >/dev/null ||
		if [ $i = libtool ] ; then
			MISSING="${MISSING}libtool-bin "
		elif [ $i = java ] ; then
			MISSING="${MISSING}openjdk-8-jdk "
		else
			MISSING="${MISSING}${i} "
		fi
done
if [ "$MISSING" ] ; then
	echo "The following packages are missing: $MISSING"
	echo "Please install via your package manager."
	exit 1
fi

# first we need to get the Android SDK and NDK
if [ ! -d "$ANDROID_NDK" ] ; then
	if [ ! -f "$NDK_BINARIES" ] ; then
		wget -q https://dl.google.com/android/repository/"$NDK_BINARIES"
	fi
	unzip -q "$NDK_BINARIES"
fi

if [ ! -d "$ANDROID_SDK"/build-tools/"${ANDROID_BUILDTOOLS_REVISION}" ] ||
   [ ! -d "$ANDROID_SDK"/platforms/"${ANDROID_PLATFORMS}" ] ||
   [ ! -d "$ANDROID_SDK"/platforms/"${ANDROID_PLATFORM}" ] ; then
	if [ ! -d "$ANDROID_SDK" ] ; then
		if [ ! -f "$SDK_TOOLS" ] ; then
			wget -q https://dl.google.com/android/repository/"$SDK_TOOLS"
		fi
		mkdir "$ANDROID_SDK"
		pushd "$ANDROID_SDK"
		unzip -q ../"$SDK_TOOLS"
		yes | tools/bin/sdkmanager --licenses > /dev/null 2>&1 || echo "d56f5187479451eabf01fb78af6dfcb131a6481e" > licenses/android-sdk-license
		cat licenses/android-sdk-license
		yes | tools/bin/sdkmanager tools platform-tools 'platforms;'"${ANDROID_PLATFORM}" 'platforms;'"${ANDROID_PLATFORMS}" 'build-tools;'"${ANDROID_BUILDTOOLS_REVISION}" > /dev/null
		echo ""
	else
		pushd "$ANDROID_SDK"
		yes | tools/bin/sdkmanager tools platform-tools 'platforms;'"${ANDROID_PLATFORM}" 'platforms;'"${ANDROID_PLATFORMS}" 'build-tools;'"${ANDROID_BUILDTOOLS_REVISION}" > /dev/null
	fi
	popd
fi

# now that we have an NDK, copy the font that we need for OnePlus phones
# due to https://bugreports.qt.io/browse/QTBUG-69494
cp "$ANDROID_SDK"/platforms/"${ANDROID_PLATFORM}"/data/fonts/Roboto-Regular.ttf "$SUBSURFACE_SOURCE"/android-mobile || exit 1

# download the Qt installer including Android bits and unpack / install
QT_DOWNLOAD_URL=https://download.qt.io/archive/qt/${QT_VERSION}/${LATEST_QT}/${QT_BINARIES}
if [ ! -d Qt/"${LATEST_QT}"/android_armv7 ] ; then
	if [ -d Qt ] ; then
		# Over writing an exsisting installation stalls the installation script,
		# rename the exsisting Qt folder and notify then user.
		mv Qt Qt_OLD
		echo "Qt installation found, backing it up to Qt_OLD."
	fi
	if [ ! -f "${QT_BINARIES}" ] ; then
		wget -q "${QT_DOWNLOAD_URL}"
	fi
	chmod +x ./"${QT_BINARIES}"
	./"${QT_BINARIES}" --platform minimal --script "$SCRIPTDIR"/qt-installer-noninteractive.qs --no-force-installations
fi

# patch the cmake / Qt5.7.1 incompatibility mentioned above
sed -i 's/set_property(TARGET Qt5::Core PROPERTY INTERFACE_COMPILE_FEATURES cxx_decltype)/# &/' Qt/"${LATEST_QT}"/android_armv7/lib/cmake/Qt5Core/Qt5CoreConfigExtras.cmake

if [ ! -z ${PREP_ONLY+x} ] ; then
	exit 0
fi

if [ ! -d subsurface/libdivecomputer/src ] ; then
	pushd subsurface
	git submodule init
	git submodule update --recursive
	popd
fi

# always reconfigure here
rm -f subsurface/libdivecomputer/configure
pushd subsurface/libdivecomputer
autoreconf --install --force
autoreconf --install --force
popd

# and now we need a monotonic build number...
if [ ! -f ./buildnr.dat ] ; then
	BUILDNR=0
else
	BUILDNR=$(cat ./buildnr.dat)
fi
BUILDNR=$((BUILDNR+1))
echo "${BUILDNR}" > ./buildnr.dat

echo "Building Subsurface-mobile for Android, build nr ${BUILDNR}"

rm -f ./subsurface-mobile-build-arm/build/outputs/apk/debug/*.apk
rm -df ./subsurface-mobile-build-arm/AndroidManifest.xml

if [ "$USE_X" ] ; then
	bash "$USE_X" "$SUBSURFACE_SOURCE"/packaging/android/build.sh -buildnr "$BUILDNR" arm $QUICK $RELEASE
	# the arm64 APK has to have a higher build number
	BUILDNR=$((BUILDNR+1))
	echo "${BUILDNR}" > ./buildnr.dat
	bash "$USE_X" "$SUBSURFACE_SOURCE"/packaging/android/build.sh -buildnr "$BUILDNR" arm64 $QUICK $RELEASE
else
	bash "$SUBSURFACE_SOURCE"/packaging/android/build.sh -buildnr "$BUILDNR" arm $QUICK $RELEASE
	# the arm64 APK has to have a higher build number
	BUILDNR=$((BUILDNR+1))
	echo "${BUILDNR}" > ./buildnr.dat
	bash "$SUBSURFACE_SOURCE"/packaging/android/build.sh -buildnr "$BUILDNR" arm64 $QUICK $RELEASE
fi

ls -l ./subsurface-mobile-build-arm/build/outputs/apk/debug/*.apk

