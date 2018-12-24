#!/bin/bash

if [ ! -z $TRAVIS_BRANCH ] && [ "$TRAVIS_BRANCH" != "master" ] ; then
	export UPLOADTOOL_SUFFIX=$TRAVIS_BRANCH
fi

# set up the release message to use
source ${TRAVIS_BUILD_DIR}/scripts/release-message.sh

echo "Submitting the folloing apk for continuous build release:"
ls -lh /android/subsurface-mobile-build-arm*/build/outputs/apk/*.apk

# get and run the upload script
wget -c https://raw.githubusercontent.com/dirkhh/uploadtool/master/upload.sh
bash ./upload.sh /android/subsurface-mobile-build-arm*/build/outputs/apk/*.apk
