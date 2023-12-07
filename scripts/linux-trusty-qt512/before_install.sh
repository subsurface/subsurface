#!/bin/bash

# prep things so we can build for Linux
# we have a custom built Qt some gives us just what we need, including QtWebKit
#

set -x

# when running this locally, set TRAVIS_BUILD_DIR to the Subsurface
# directory inside your Windows build tree
TRAVIS_BUILD_DIR=${TRAVIS_BUILD_DIR:-$PWD}

git fetch --unshallow || true # if running locally, unshallow could fail
git pull --tags
git submodule init
git describe --match "v[0-9]*"

# make sure we have libdivecomputer
echo "Get libdivecomputer"
cd ${TRAVIS_BUILD_DIR}
git submodule update --recursive
cd libdivecomputer
autoreconf --install
autoreconf --install

export QT_ROOT=/usr/local/Qt/5.12.4

cd ${TRAVIS_BUILD_DIR}/..

# start the container and keep it running
docker run -v $PWD/subsurface:/subsurface --name=trusty-qt512 -w / -d dirkhh/trusty-qt512:0.7 /bin/sleep 60m
