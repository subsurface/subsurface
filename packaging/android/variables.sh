#!/bin/bash
# When changing Qt version remember to update the 
# qt-installer-noninteractive file as well.
QT_VERSION=5.13
LATEST_QT=5.13.1
NDK_VERSION=r18b
SDK_VERSION=4333796
ANDROID_BUILDTOOLS_REVISION=28.0.3
ANDROID_PLATFORM_LEVEL=21
ANDROID_PLATFORM=android-21
ANDROID_PLATFORMS=android-27
ANDROID_NDK=android-ndk-${NDK_VERSION}
ANDROID_SDK=android-sdk-linux
# OpenSSL also has an entry in get-dep-lib.sh line 103
# that needs to be updated as well.
OPENSSL_VERSION=1.1.1d
