#!/bin/bash
# When changing Qt version remember to update the 
# qt-installer-noninteractive file as well.
QT_VERSION=6.7
LATEST_QT=6.7.2
NDK_VERSION=26.1.10909125
ANDROID_BUILDTOOLS_REVISION=29.0.3
ANDROID_PLATFORM_LEVEL=23
ANDROID_PLATFORM=android-31
ANDROID_PLATFORMS=android-34
ANDROID_NDK=ndk/${NDK_VERSION}
# OpenSSL also has an entry in get-dep-lib.sh line 103
# that needs to be updated as well.
OPENSSL_VERSION=3.0
