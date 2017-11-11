#!/bin/bash

set -x

# Travis only pulls shallow repos. But that messes with git describe.
# Sorry Travis, fetching the whole thing and the tags as well...
git fetch --unshallow
git pull --tags
git describe

# grab our own custom MXE environment
cd ${TRAVIS_BUILD_DIR}/..
echo "Downloading prebuilt MXE environment from Subsurface-divelog.org"
wget -q http://subsurface-divelog.org/downloads/mxe-994ad473.tar.xz
mkdir -p mxe
cd mxe
tar xJf ../mxe-994ad473.tar.xz

# hack around path dependency - needs to be fixed
sudo mkdir -p /data/winqt551/
sudo ln -s ${TRAVIS_BUILD_DIR}/../mxe /data/winqt551/mxe-current
ls -l /data/winqt551/mxe-current/usr

# libdivecomputer uses the wrong include path for libusb
# the pkgconfig file for libusb already gives the include path as
# ../include/libusb-1.0  yet libdivecomputer wants to use
# include <libusb-1.0/libusb.h>
cd usr/i686-w64-mingw32.shared/include/libusb-1.0
ln -s . libusb-1.0
cd ${TRAVIS_BUILD_DIR}/..

# now set up our other dependencies

CURRENT_LIBZIP="1.2.0"
CURRENT_HIDAPI="hidapi-0.7.0"
CURRENT_LIBUSB="v1.0.21"
CURRENT_LIBGIT2="v0.26.0"


#this will soon be a git submodule
echo "Get libdivecomputer"
cd ${TRAVIS_BUILD_DIR}/..
git clone -b Subsurface-branch https://github.com/Subsurface-divelog/libdc.git libdivecomputer
cd libdivecomputer
if [ ! -f ./configure ] ; then
        autoreconf --install .
        autoreconf --install . # not a typo - somehow/sometimes libdivecomputer needs to run this twice. Don't ask.
fi

echo "Get libusb"
cd ${TRAVIS_BUILD_DIR}/..
git clone https://github.com/libusb/libusb
cd libusb
if ! git checkout $CURRENT_LIBUSB ; then
	echo "Can't find the right tag in libusb - giving up"
	exit 1
fi

echo "Get libgit2"
cd ${TRAVIS_BUILD_DIR}/..
git clone https://github.com/libgit2/libgit2.git
cd libgit2
if ! git checkout $CURRENT_LIBGIT2 ; then
	echo "Can't find the right tag in libgit2 - giving up"
	exit 1
fi

echo "Get googlemaps"
cd ${TRAVIS_BUILD_DIR}/..
git clone https://github.com/Subsurface-divelog/googlemaps.git

echo "Get Grantlee"
cd ${TRAVIS_BUILD_DIR}/..
git clone https://github.com/steveire/grantlee.git
cd grantlee
if ! git checkout v5.0.0 ; then
	echo "can't check out v5.0.0 of grantlee -- giving up"
	exit 1
fi
