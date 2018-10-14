#!/bin/bash

if [ ! -z $TRAVIS_BRANCH ] && [ "$TRAVIS_BRANCH" != "master" ] ; then
	export UPLOADTOOL_SUFFIX=$TRAVIS_BRANCH
fi

cd ${TRAVIS_BUILD_DIR}/../win32/subsurface

echo "Submitting the following Windows files for continuous build release:"
find . -name subsurface\*.exe*

# set up the release message to use
source ${TRAVIS_BUILD_DIR}/scripts/release-message.sh

# for debugging, let's look around a bit
ls -la

# the win32 dir is owned by root, as is all its content (because that's
# the user inside the docker container, I guess. So let's create upload.sh
# in the TRAVIS_BUILD_DIR
cd ${TRAVIS_BUILD_DIR}

# get and run the upload script
wget -c https://raw.githubusercontent.com/dirkhh/uploadtool/master/upload.sh
bash ./upload.sh ${TRAVIS_BUILD_DIR}/../win32/subsurface/subsurface*.exe*

# upload smtk2ssrf
bash ./upload.sh ${TRAVIS_BUILD_DIR}/../win32/smtk-import/smtk2ssrf*.exe*
