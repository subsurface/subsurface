#!/bin/bash

if [ ! -z $TRAVIS_BRANCH ] && [ "$TRAVIS_BRANCH" != "master" ] ; then
	export UPLOADTOOL_SUFFIX=$TRAVIS_BRANCH
fi

echo "Submitting the folloing AppImage for continuous build release:"
ls -lh Subsurface*.AppImage

# get and run the upload script
wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
bash ./upload.sh Subsurface*.AppImage Subsurface*.AppImage.zsync

