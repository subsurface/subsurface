#!/bin/bash

set -x

# Travis only pulls shallow repos. But that messes with git describe.
# Sorry Travis, fetching the whole thing and the tags as well...
git fetch --unshallow
git pull --tags
git describe

# grab our own custom MXE environment
pushd ${TRAVIS_BUILD_DIR}/..
echo "Downloading prebuilt MXE environment from Subsurface-divelog.org"
wget -q http://subsurface-divelog.org/downloads/mxe-994ad473.tar.xz
mkdir -p mxe
tar xJ -C mxe -f mxe-994ad473.tar.xz

# hack around path dependency - needs to be fixed
sudo mkdir -p /data/winqt551/
sudo ln -s ${TRAVIS_BUILD_DIR}/../mxe /data/winqt551/mxe-current
ls -l /data/winqt551/mxe-current/usr
sudo ln -s ${TRAVIS_BUILD_DIR}/../mxe /usr/src/mxe
popd

# now set up our other dependencies

CURRENT_LIBZIP="1.2.0"
CURRENT_HIDAPI="hidapi-0.7.0"
CURRENT_LIBUSB="v1.0.21"
CURRENT_LIBGIT2="v0.26.0"

# make sure we have libdivecomputer
echo "Get libdivecomputer"
cd ${TRAVIS_BUILD_DIR}
git submodule update --recursive
cd libdivecomputer
autoreconf --install
autoreconf --install

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
git clone https://github.com/Subsurface/googlemaps.git

echo "Get mdbtools"
cd ${TRAVIS_BUILD_DIR}/..
git clone https://github.com/brianb/mdbtools.git

# get prebuilt mxe libraries for mdbtools and glib.
# do not overwrite upstream prebuilt mxe binaries if there is any coincidence.
wget https://www.dropbox.com/s/842skyusb96ii1u/mxe-static-minimal-994ad473.tar.xz
[[ ! -f mxe-static-minimal-994ad473.tar.xz ]] && exit 1
cd mxe
tar -xJf ../mxe-static-minimal-994ad473.tar.xz --skip-old-files
ls -al usr/
