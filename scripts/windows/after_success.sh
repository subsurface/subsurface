#!/bin/bash

if [ ! -z $TRAVIS_BRANCH ] && [ "$TRAVIS_BRANCH" != "master" ] ; then
	export UPLOADTOOL_SUFFIX=$TRAVIS_BRANCH
fi

cd ${TRAVIS_BUILD_DIR}/../win32/subsurface

echo "Submitting the folloing Windows installer for continuous build release:"
find . -name subsurface\*.exe

# get and run the upload script
wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
bash ./upload.sh subsurface*.exe

