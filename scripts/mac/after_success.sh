#!/bin/bash

if [ ! -z $TRAVIS_BRANCH ] && [ "$TRAVIS_BRANCH" != "master" ] ; then
	export UPLOADTOOL_SUFFIX=$TRAVIS_BRANCH
fi

# same git version magic as in the Makefile
# for the naming of the app
export VERSION=$(cd ${TRAVIS_BUILD_DIR}; ./scripts/get-version linux)


cd ${TRAVIS_BUILD_DIR}/build
zip -r -y Subsurface-$VERSION.app.zip Subsurface.app

echo "Submitting the folloing App for continuous build release:"
ls -lh Subsurface-$VERSION.app.zip

# get and run the upload script
wget -c https://github.com/probonopd/uploadtool/raw/master/upload.sh
bash ./upload.sh Subsurface-$VERSION.app.zip

