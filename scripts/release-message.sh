#!/bin/bash

# create a more useful release message body

export VERSION=$(cd ${TRAVIS_BUILD_DIR}; ./scripts/get-version linux)
T_BUILD_REF="Travis CI build log: https://travis-ci.org/Subsurface-divelog/subsurface/builds/$TRAVIS_BUILD_ID/\n\n"
WIN_BINS="subsurface.exe and subsurface.exe.debug are just the Subsurface executable for this build, the full Windows installer is subsurface-$VERSION.exe.\n\n"
MAC_ZIP="Subsurface-$VERSION.app.zip is a zip archive containing an unsigned app folder; you will have to override Mac security settings in order to be able to run this app.\n\n"
ANDROID_APK="The Android APK is not signed with the release key, most Android phones will force you to uninstall Subsurface-mobile before you can install this APK if you already have an official binary installed on your Android device.\n\n"
MISSING_BINARIES="While the continuous builds are running not all binaries may be posted here - please reload the page in a few minutes if the binary you are looking for is missing.\n"
export UPLOADTOOL_BODY=$T_BUILD_REF$WIN_BINS$MAC_ZIP$ANDROID_APK$MISSING_BINARIES
