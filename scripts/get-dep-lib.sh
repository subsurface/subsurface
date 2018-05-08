#!/bin/bash
#

# set version of 3rd party libraries
CURRENT_LIBZIP="1.2.0"
CURRENT_LIBGIT2="v0.26.0"
CURRENT_HIDAPI="hidapi-0.7.0"
CURRENT_LIBCURL="curl-7_54_1"
CURRENT_LIBUSB="v1.0.21"
CURRENT_OPENSSL="OpenSSL_1_1_0f"
CURRENT_LIBSSH2="libssh2-1.8.0"
CURRENT_XSLT="v1.1.29"
CURRENT_SQLITE="3190200"
CURRENT_LIBXML2="v2.9.4"
CURRENT_LIBFTDI="1.3"



# deal with all the command line arguments
if [ $# -ne 2 ] ; then
	echo "wrong number of parameters, format:"
	echo "get-dep-lib <platform> <install dir>"
	echo "where platform is one of scripts, ios or android"
	echo "(the name of the directory where build.sh resides)"
	exit -1
fi

PLATFORM=$1
INSTDIR=$2
if [ ! -d ${INSTDIR} ] ; then
	echo "creating dir"
	mkdir -p ${INSTDIR}
fi

case ${PLATFORM} in
	scripts)
		BUILD="libzip libgit2 googlemaps hidapi libcurl libusb openssl libssh2"
		;;
	ios)
		BUILD="libzip libgit2 googlemaps libxslt"
		;;
	android)
		BUILD="libzip libgit2 googlemaps libxslt sqlite libxml2 openssl libftdi libusb"
		;;
	*)
		echo "Unknown platform ${PLATFORM}, choose between native, ios or android"
		;;
esac

# show what you are doing and stop when things break
set -x
set -e

# get ready to download needed sources
cd ${INSTDIR}

if [[ $BUILD = *"libcurl"* ]]; then
	if [ ! -d libcurl ] ; then
		git clone https://github.com/curl/curl libcurl
	fi
	pushd libcurl
	git fetch origin
	if ! git checkout $CURRENT_LIBCURL ; then
		echo "Can't find the right tag in libcurl - giving up"
		exit -1
	fi
	popd
fi

if [[ $BUILD = *"libftdi"* ]]; then
	if [ ! -d libftdi1 ] ; then
		curl -O https://www.intra2net.com/en/developer/libftdi/download/libftdi1-${CURRENT_LIBFTDI}.tar.bz2
		tar -jxf libftdi1-${CURRENT_LIBFTDI}.tar.bz2
	mv libftdi1-${CURRENT_LIBFTDI} libftdi1
	fi
fi

if [[ $BUILD = *"libgit2"* ]]; then
	if [ ! -d libgit2 ] ; then
		git clone https://github.com/libgit2/libgit2.git
	fi
	pushd libgit2
	git fetch origin
	if ! git checkout ${CURRENT_LIBGIT2} ; then
		echo "Can't find the right tag in libgit2 - giving up"
		exit -1
	fi
	popd
fi

if [[ $BUILD = *"libssh2"* ]]; then
	if [ ! -d libssh2 ] ; then
		git clone https://github.com/libssh2/libssh2
	fi
	pushd libssh2
	git fetch origin
	if ! git checkout $CURRENT_LIBSSH2 ; then
		echo "Can't find the right tag in libssh2 - giving up"
		exit -1
	fi
	popd
fi

if [[ $BUILD = *"libusb"* ]]; then
	if [ ! -d libusb ] ; then
		git clone https://github.com/libusb/libusb
	fi
	pushd libusb
	git fetch origin
	if ! git checkout $CURRENT_LIBUSB ; then
		echo "Can't find the right tag in libusb - giving up"
		exit -1
	fi
	popd
fi

if [[ $BUILD = *"libxml2"* ]]; then
	if [ ! -d libxml2 ] ; then
		git clone https://github.com/GNOME/libxml2.git
	fi
	pushd libxml2
	git fetch origin
	if ! git checkout $CURRENT_LIBXML2 ; then
		echo "Can't find the right tag in libxml2 - giving up"
		exit -1
	fi
	popd
fi

if [[ $BUILD = *"libxslt"* ]]; then
	if [ ! -d libxslt ] ; then
		git clone https://github.com/GNOME/libxslt.git
	fi
	pushd libxslt
	git fetch origin
	if ! git checkout $CURRENT_LIBXSLT ; then
		echo "Can't find the right tag in libxslt - giving up"
		exit -1
	fi
	popd
fi

if [[ $BUILD = *"libzip"* ]]; then
	if [ ! -d libzip ] ; then
		curl -O https://libzip.org/download/libzip-${CURRENT_LIBZIP}.tar.gz
		tar xzf libzip-${CURRENT_LIBZIP}.tar.gz
		mv libzip-${CURRENT_LIBZIP} libzip
	fi
fi

if [[ $BUILD = *"googlemaps"* ]]; then
	if [ ! -d googlemaps ] ; then
		git clone https://github.com/Subsurface-divelog/googlemaps.git
	fi
	pushd googlemaps
	git fetch origin
	git checkout master
	git pull --rebase
	popd
fi

if [[ $BUILD = *"hidapi"* ]]; then
	if [ ! -d hidapi ] ; then
		git clone https://github.com/signal11/hidapi
	fi
	pushd hidapi
	git fetch origin
	if ! git checkout $CURRENT_HIDAPI ; then
		echo "Can't find the right tag in hidapi - giving up"
		exit -1
	fi
	popd
fi

if [[ $BUILD = *"openssl"* ]]; then
	if [ ! -d openssl ] ; then
		git clone https://github.com/openssl/openssl
	fi
	pushd openssl
	git fetch origin
	if ! git checkout $CURRENT_OPENSSL ; then
		echo "Can't find the right tag in openssl - giving up"
		exit -1
	fi
	popd
fi

if [[ $BUILD = *"sqlite"* ]]; then
	if [ ! -d sqlite ] ; then
		curl -O http://www.sqlite.org/2017/sqlite-autoconf-${CURRENT_SQLITE}.tar.gz
		tar -zxf sqlite-autoconf-${CURRENT_SQLITE}.tar.gz
		mv sqlite-autoconf-${CURRENT_SQLITE} sqlite
	fi
fi

exit 0
