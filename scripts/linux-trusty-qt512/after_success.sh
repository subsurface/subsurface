#!/bin/bash

cd ${TRAVIS_BUILD_DIR}

if [ ! -z $TRAVIS_BRANCH ] && [ "$TRAVIS_BRANCH" != "master" ] ; then
	export UPLOADTOOL_SUFFIX=$TRAVIS_BRANCH
fi

# set up the release message to use
source ./scripts/release-message.sh

echo "Submitting the folloing AppImage for continuous build release:"
find . -name "Subsurface*.AppImage"

# get and run the upload script
wget -c https://raw.githubusercontent.com/dirkhh/uploadtool/master/upload.sh

# don't fail if the zsync file is missing
if [ -f Subsurface*.AppImage.zsync ] ; then
	bash ./upload.sh Subsurface*.AppImage Subsurface*.AppImage.zsync
else
	bash ./upload.sh Subsurface*.AppImage
fi
