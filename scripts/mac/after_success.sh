#!/bin/bash

if [ ! -z $TRAVIS_BRANCH ] && [ "$TRAVIS_BRANCH" != "master" ] ; then
	export UPLOADTOOL_SUFFIX=$TRAVIS_BRANCH
fi

# set up the release message to use
source ${TRAVIS_BUILD_DIR}/scripts/release-message.sh

cd ${TRAVIS_BUILD_DIR}/build
zip -r -y Subsurface-$VERSION.app.zip Subsurface.app

echo "Submitting the folloing App for continuous build release:"
ls -lh Subsurface-$VERSION.app.zip

# get and run the upload script
wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
bash ./upload.sh Subsurface-$VERSION.app.zip

