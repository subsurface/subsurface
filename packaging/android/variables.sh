#!/bin/bash
# When changing Qt version remember to update the 
# qt-installer-noninteractive file as well.
QT_VERSION=5.11
LATEST_QT=5.11.1
NDK_VERSION=r14b
SDK_VERSION=3859397
ANDROID_BUILDTOOLS_REVISION=25.0.3
ANDROID_PLATFORMS=android-27
ANDROID_NDK=android-ndk-${NDK_VERSION}
ANDROID_SDK=android-sdk-linux
# OpenSSL also has an entry in get-dep-lib.sh line 103
# that needs to be updated as well.
OPENSSL_VERSION=1.0.2o
