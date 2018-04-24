#!/bin/bash

set -x
set -e

# try to get rid of the insane debug crap
unalias -a
unset -f rvm_debug
unset -f cd
unset -f pushd
unset -f popd

# Travis only pulls shallow repos. But that messes with git describe.
# Sorry Travis, fetching the whole thing and the tags as well...
git fetch --unshallow
git pull --tags
git describe

git submodule init
git submodule update --recursive

pushd libdivecomputer
autoreconf --install
autoreconf --install
popd

# prep things so we can build for iOS
# we have a custom built Qt some gives us just what we need

pushd ${TRAVIS_BUILD_DIR}/..

echo "Get custom Qt build and unpack it"
curl --output ./Qt5.10.1-iOS.tar.xz \
                https://storage.googleapis.com/travis-cache/Qt5.10.1-iOS.tar.xz
tar xJf Qt5.10.1-iOS.tar.xz

# our scripts assume that there are two convenience links - arguably this
# should be fixed in the scripts, but...
ln -s Qt5.10.1 Qt
cd subsurface/packaging/ios
ln -s ${TRAVIS_BUILD_DIR}/../Qt Qt

popd
