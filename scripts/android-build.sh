#!/bin/bash

# run this in the top level folder you want to create Android binaries in
#
# it seems that with Qt5.7.1 and current cmake there is an odd bug where
# cmake fails reporting :No known features for CXX compiler "GNU". In that
# case simly comment out the "set(property(TARGET Qt5::Core PROPERTY...)"
# at line 101 of
# Qt/5.7/android_armv7/lib/cmake/Qt5Core/Qt5CoreConfigExtras.cmake

exec 1> >(tee ./build.log) 2>&1

USE_X=`case "$-" in *x*) echo "-x" ;; esac`

# these are the current versions for Qt, Android SDK & NDK:

QT_VERSION=5.7
LATEST_QT=5.7.1
NDK_VERSION=r13b
SDK_VERSION=r25.2.3

ANDROID_NDK=android-ndk-${NDK_VERSION}
ANDROID_SDK=android-sdk-linux

PLATFORM=$(uname)

if [ "$PLATFORM" = "Linux" ] ; then
	QT_BINARIES=qt-opensource-linux-x64-android-${LATEST_QT}.run
	NDK_BINARIES=${ANDROID_NDK}-linux-x86_64.zip
	SDK_TOOLS=tools_${SDK_VERSION}-linux.zip
else
	echo "only on Linux so far"
	exit 1
fi

# download the Qt installer including Android bits and unpack / install
QT_DOWNLOAD_URL=https://download.qt.io/archive/qt/${QT_VERSION}/${LATEST_QT}/${QT_BINARIES}
if [ ! -f ${QT_BINARIES} ] ; then
	wget ${QT_DOWNLOAD_URL}
	echo "In the binary installer select $(pwd)/${LATEST_QT} as install directory"
	bash ./${QT_BINARIES}
fi

[ -e Qt ] || ln -s Qt${LATEST_QT} Qt

# next we need to get the Android SDK and NDK
if [ ! -d $ANDROID_NDK ] ; then
	if [ ! -f $NDK_BINARIES ] ; then
		echo "Please download the Android NDK $NDK_BINARIES from Google's servers and place it in this directory"
		exit
	fi
	unzip $NDK_BINARIES
fi

if [ ! -d $ANDROID_SDK ] ; then
	if [ ! -f $SDK_TOOLS ] ; then
		echo "Please download the Android SDK $SDK_TOOLS from Google's servers and place it in this directory"
		exit
	fi
	mkdir $ANDROID_SDK
	pushd $ANDROID_SDK
	unzip ../$SDK_TOOLS
	bash tools/android update sdk --no-ui -a -t 1,2,3,33
	popd
fi

# ok, now we have Qt, SDK, and NDK - let's get us some Subsurface
if [ ! -d subsurface ] ; then
	git clone git://github.com/Subsurface-divelog/subsurface
fi
pushd subsurface
git pull --rebase
popd

if [ ! -d libdivecomputer ] ; then
	git clone -b Subsurface-branch git://github.com/Subsurface-divelog/libdc libdivecomputer
	pushd libdivecomputer
	autoreconf --install
	autoreconf --install
	popd
fi
pushd libdivecomputer
git pull --rebase
if ! git checkout Subsurface-branch ; then
	echo "can't check out the Subsurface-branch branch of libdivecomputer -- giving up"
	exit 1
fi
popd


# and now we need a monotonic build number...
if [ ! -f "./buildnr.dat" ] ; then
	BUILDNR=0
else
	BUILDNR=$(cat ./buildnr.dat)
fi
BUILDNR=`expr ${BUILDNR} + 1`
echo "${BUILDNR}" > ./buildnr.dat

echo "Building Subsurface-mobile ${VERSION} for Android, build nr ${BUILDNR} as Subsurface-mobile-$VERSION-${NAME}arm.apk"

if [ "$1" = "release" ] || [ "$1" = "Release" ] || [ "$1" = "debug" ] || [ "$1" = "Debug" ] ; then
	RELEASE=$1
	shift
else
	RELEASE=Debug
fi

rm -f ./subsurface-mobile-build-arm/bin/QtApp-debug.apk
rm -d ./subsurface-mobile-build-arm/AndroidManifest.xml
rm -d ./subsurface-mobile-build-arm/bin/AndroidManifest.xml
bash $USE_X subsurface/packaging/android/build.sh $RELEASE -buildnr ${BUILDNR} arm $@ 

