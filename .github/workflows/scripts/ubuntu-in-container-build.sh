#!/bin/bash

# this is used to do a standard build of Subsurface (including tests) in a "random"
# Ubuntu container (I assume we will keep moving this to the latest Ubuntu)

set -x
set -e

echo "--------------------------------------------------------------"
echo "update distro and install dependencies"

apt-get update
apt-get upgrade -y
DEBIAN_FRONTEND=noninteractive apt-get install -y -q --force-yes \
	autoconf automake cmake g++ git libcrypto++-dev libcurl4-gnutls-dev \
	libgit2-dev libqt5qml5 libqt5quick5 libqt5svg5-dev \
	libqt5webkit5-dev libsqlite3-dev libssh2-1-dev libssl-dev libssl-dev \
	libtool libusb-1.0-0-dev libxml2-dev libxslt1-dev libzip-dev make \
	pkg-config qml-module-qtlocation qml-module-qtpositioning \
	qml-module-qtquick2 qt5-default qt5-qmake qtchooser qtconnectivity5-dev \
	qtdeclarative5-dev qtdeclarative5-private-dev qtlocation5-dev \
	qtpositioning5-dev qtscript5-dev qttools5-dev qttools5-dev-tools \
	qtquickcontrols2-5-dev xvfb

echo "--------------------------------------------------------------"
echo "building mobile"

# first make sure that no one broke Subsurface-mobile
bash -e -x subsurface/scripts/build.sh -mobile

pushd subsurface/build-mobile
xvfb-run --auto-servernum make check
popd

echo "--------------------------------------------------------------"
echo "building desktop"

# now build our AppImage
bash -e -x subsurface/scripts/build.sh -desktop -build-with-webkit

pushd subsurface/build
xvfb-run --auto-servernum make check
popd

