#!/bin/bash
# When changing Qt version remember to update the 
# qt-installer-noninteractive file as well.
QT_VERSION=5.15
LATEST_QT=5.15.1
NDK_VERSION=21.3.6528147
ANDROID_BUILDTOOLS_REVISION=29.0.3
ANDROID_PLATFORM_LEVEL=21
ANDROID_PLATFORM=android-21
ANDROID_PLATFORMS=android-29
ANDROID_NDK=ndk/${NDK_VERSION}
# OpenSSL also has an entry in get-dep-lib.sh line 103
# that needs to be updated as well.
OPENSSL_VERSION=1.1.1h
