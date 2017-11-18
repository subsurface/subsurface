#!/bin/bash

# don't run this for pull requests
if [ "$TRAVIS_EVENT_TYPE" == "pull_request" ] ; then
  exit 0;
fi

if [ ! -z $UPLOADTOOL_SUFFIX ] ; then
  if [ "$UPLOADTOOL_SUFFIX" = "$TRAVIS_TAG" ] ; then
    RELEASE_NAME=$TRAVIS_TAG
    RELEASE_TITLE="Release build ($TRAVIS_TAG)"
    is_prerelease="false"
  else
    RELEASE_NAME="continuous-$UPLOADTOOL_SUFFIX"
    RELEASE_TITLE="Continuous build ($UPLOADTOOL_SUFFIX)"
    is_prerelease="true"
  fi
else
  RELEASE_NAME="continuous" # Do not use "latest" as it is reserved by GitHub
  RELEASE_TITLE="Continuous build"
  is_prerelease="true"
fi

# update the Body of the Release to be more interesting

VERSION=$(cd ${TRAVIS_BUILD_DIR}; ./scripts/get-version linux)
T_BUILD_REF="Travis CI build log: https://travis-ci.org/Subsurface-divelog/subsurface/builds/$TRAVIS_BUILD_ID/\n\n"
WIN_BINS="subsurface.exe and subsurface.exe.debug are just the Subsurface executable for this build, the full Windows installer is subsurface-$VERSION.exe.\n\n"
MAC_ZIP="Subsurface-$VERSION.app.zip is a zip archive containing an unsigned app folder; you will have to override Mac security settings in order to be able to run this app.\n\n"
ANDROID_APK="The Android APK is not signed with the release key, most Android phones will force you to uninstall Subsurface-mobile before you can install this APK if you already have an official binary installed on your Android device.\n\n"
MISSING_BINARIES="While the continuous builds are running not all binaries may be posted here - please reload the page in a few minutes if the binary you are looking for is missing\n."
BODY=$T_BUILD_REF$WIN_BINS$MAC_ZIP$ANDROID_APK$MISSING_BINARIES

release_id=$(curl https://api.github.com/repos/Subsurface-divelog/subsurface/releases/tags/${RELEASE_NAME} | grep "\"id\":" | head -n 1 | tr -s " " | cut -f 3 -d" " | cut -f 1 -d ",")
release_infos=$(curl -H "Authorization: token ${GITHUB_TOKEN}" --request PATCH \
       --data '{"tag_name": "'"$RELEASE_NAME"'","name": "'"$RELEASE_TITLE"'","body": "'"$BODY"'"}' "https://api.github.com/repos/Subsurface-divelog/subsurface/releases/${release_id}")

echo $release_infos
