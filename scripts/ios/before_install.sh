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
git describe --match "v[0-9]*"

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
curl --output ./Qt-5.11.1-ios.tar.xz \
                https://f002.backblazeb2.com/file/Subsurface-Travis/Qt-5.11.1-ios.tar.xz
md5 ./Qt-5.11.1-ios.tar.xz

mkdir -p Qt/5.11.1

tar -xJ -C Qt/5.11.1 -f Qt-5.11.1-ios.tar.xz

# our scripts assume that there is a convenience link
cd subsurface/packaging/ios
ln -s ${TRAVIS_BUILD_DIR}/../Qt Qt

popd
