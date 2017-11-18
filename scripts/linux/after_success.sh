#!/bin/bash

if [ ! -z $TRAVIS_BRANCH ] && [ "$TRAVIS_BRANCH" != "master" ] ; then
	export UPLOADTOOL_SUFFIX=$TRAVIS_BRANCH
fi

# set up the release message to use
source ${TRAVIS_BUILD_DIR}/scripts/release-message.sh

echo "Submitting the folloing AppImage for continuous build release:"
ls -lh Subsurface*.AppImage

# get and run the upload script
wget -c https://github.com/dirkhh/uploadtool/raw/master/upload.sh
bash ./upload.sh Subsurface*.AppImage Subsurface*.AppImage.zsync

