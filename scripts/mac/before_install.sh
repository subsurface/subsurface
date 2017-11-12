#!/bin/bash

set -x

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

# for our build we need an updated Homebrew with a few more components
# installed. Since the Travis cache doesn't seem to work, we put it on
# our own server
if curl --output /dev/null --silent --head --fail \
	http://subsurface-divelog.org/downloads/TravisMacBuildCache.tar.xz
then
	echo "Download Homebrew with all our packages and overwritw /usr/local"
	curl --output ${TRAVIS_BUILD_DIR}/TravisMacBuildCache.tar.xz \
		https://storage.googleapis.com/travis-cache/TravisMacBuildCache.tar.xz
#	curl --output ${TRAVIS_BUILD_DIR}/TravisMacBuildCache.tar.xz \
#		http://subsurface-divelog.org/downloads/TravisMacBuildCache.tar.xz
	sudo tar xJfC ${TRAVIS_BUILD_DIR}/TravisMacBuildCache.tar.xz /tmp
	sudo mv /usr/local /usr/local2
	sudo mv /tmp/usr/local /usr/local
else
	echo "Cannot find TravisMacBuildCache: recreate it by first updating Homebrew"
	brew update
	echo "Updated Homebrew, now get our dependencies brewed"
	brew install xz hidapi libusb libxml2 libxslt libzip openssl pkg-config libgit2
	tar cf - /usr/local | xz -v -z -0 --threads=0 > ${TRAVIS_BUILD_DIR}/TravisMacBuildCache.tar.xz
	echo "Sending new cache to transfer.sh - move it into place, please"
	ls -lh ${TRAVIS_BUILD_DIR}/TravisMacBuildCache.tar.xz
	curl --upload-file  ${TRAVIS_BUILD_DIR}/TravisMacBuildCache.tar.xz https://transfer.sh/TravisMacBuildCache.tar.xz
fi

# HACK - needs to be part of cache
brew install libssh2

# libdivecomputer uses the wrong include path for libusb and hidapi
# the pkgconfig file for libusb/hidapi already gives the include path as
# ../include/libusb-1.0 (../include/hidapi) yet libdivecomputer wants to use
# include <libusb-1.0/libusb.h> and include <hidapi/hidapi.h>

sudo ln -s /usr/local/include/libusb-1.0 /usr/local/include/libusb-1.0/libusb-1.0
sudo ln -s /usr/local/include/hidapi /usr/local/include/libusb-1.0/hidapi

# prep things so we can build for Mac
# we have a custom built Qt some gives us just what we need, including QtWebKit
#
# we should just build and install this into /usr/local/ as well and have
# it all be part of the cache...

pushd ${TRAVIS_BUILD_DIR}

mkdir -p Qt/5.9.1

echo "Get custom Qt build and unpack it"
curl --output ${TRAVIS_BUILD_DIR}/Qt-5.9.1-mac.tar.xz \
                https://storage.googleapis.com/travis-cache/Qt-5.9.1-mac.tar.xz
# wget -q http://subsurface-divelog.org/downloads/Qt-5.9.1-mac.tar.xz
tar -xJ -C Qt/5.9.1 -f Qt-5.9.1-mac.tar.xz

sudo mkdir -p /Users/hohndel
sudo ln -s ${TRAVIS_BUILD_DIR}//Qt/5.9.1 /Users/hohndel/Qt5.9.1
ls -l /Users/hohndel
