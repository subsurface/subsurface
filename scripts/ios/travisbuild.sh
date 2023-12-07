#!/bin/bash

set -x
set -e

# this gets executed by Travis when building for iOS
# it gets started from inside the subsurface directory

GITVERSION=$(git describe --match "v[0-9]*" --abbrev=12 | sed -e 's/-g.*$// ; s/^v//')
VERSION=$(echo $GITVERSION | sed -e 's/-/./')

echo "preparing dependencies for Subsurface-mobile ${VERSION} for iOS"

cd packaging/ios
bash -x build.sh -simulator

echo "now it's time to build Subsurface-mobile ${VERSION} for iOS"

cd build-Subsurface-mobile-*for_iOS-Release

sed -i.bak 's/-Wall/-Wno-everything/' Makefile

make -j4

# we don't even attempt to create an ipa on Travis
