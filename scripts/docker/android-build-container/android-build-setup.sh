#!/bin/bash -eu

# run this in the top level folder you want to create Android binaries in
#
# the script requires the Android Command Line Tools to be in cmdline-tools/bin
#

exec 1> >(tee ./build.log) 2>&1

if [ "$(uname)" != Linux ] ; then
	echo "only on Linux so far"
	exit 1
fi

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"

# these are the current versions for Qt, Android SDK & NDK:
source "$SCRIPTDIR"/variables.sh

# make sure we were started from the right directory
if [ ! -d cmdline-tools ] ; then
	echo "Start from within your Android build directory which needs to already include the Android Cmdline Tools"
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
export JAVA_HOME=/usr
export ANDROID_HOME=$(pwd)
export PATH=$ANDROID_HOME/cmdline-tools/bin:/usr/local/bin:/bin:/usr/bin
rm -rf cmdline-tools/latest
yes | sdkmanager --sdk_root="$ANDROID_HOME" "ndk;$NDK_VERSION" "cmdline-tools;latest" "platform-tools" "platforms;$ANDROID_PLATFORMS" "build-tools;$ANDROID_BUILDTOOLS_REVISION"
yes | sdkmanager --sdk_root=/android --licenses

# next check that Qt is installed
if [ ! -d "$LATEST_QT" ] ; then
	pip3 install aqtinstall
	$HOME/.local/bin/aqt install -O /android "$LATEST_QT" linux android
fi

# now that we have an NDK, copy the font that we need for OnePlus phones
# due to https://bugreports.qt.io/browse/QTBUG-69494
cp "$ANDROID_HOME"/platforms/"$ANDROID_PLATFORMS"/data/fonts/Roboto-Regular.ttf "$SCRIPTDIR"/../../android-mobile || exit 1

echo "things are set up for the Android build"
