#!/bin/bash

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

T_BUILD_REF="Travis CI build log: https://travis-ci.org/Subsurface-divelog/subsurface/builds/$TRAVIS_BUILD_ID/\n"
WIN_BINS="subsurface.exe and subsurface.exe.debug are just the Subsurface executable for this build, the full Windows installer is subsurface-$VERSION.exe.\n"
MAC_ZIP="Subsurface-$VERSION.app.zip is a zip archive containing an unsigned app folder; you will have to override Mac security settings in order to be able to run this app.\n"
MISSING_BINS="The Android APK is not yet automatically created on Travis - try looking for the latest at http://subsurface-divelog.org/downloads/test."
BODY=$T_BUILD_REF$WIN_BINS$MAC_ZIP$MISSING_BINS

release_id=$(curl https://api.github.com/repos/Subsurface-divelog/subsurface/releases/tags/${RELEASE_NAME} | grep "\"id\":" | head -n 1 | tr -s " " | cut -f 3 -d" " | cut -f 1 -d ",")
release_infos=$(curl -H "Authorization: token ${GITHUB_TOKEN}" --request PATCH \
       --data '{"tag_name": "'"$RELEASE_NAME"'","name": "'"$RELEASE_TITLE"'","body": "'"$BODY"'"}' "https://api.github.com/repos/Subsurface-divelog/subsurface/releases/${release_id}")

echo $release_infos
