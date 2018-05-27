#!/bin/bash
#

# set version of 3rd party libraries
CURRENT_LIBZIP="1.2.0"
CURRENT_LIBGIT2="v0.26.0"
CURRENT_HIDAPI="hidapi-0.7.0"
CURRENT_LIBCURL="curl-7_54_1"
CURRENT_LIBUSB="v1.0.21"
CURRENT_OPENSSL="OpenSSL_1_1_0h"
CURRENT_LIBSSH2="libssh2-1.8.0"
CURRENT_XSLT="v1.1.29"
CURRENT_SQLITE="3190200"
CURRENT_LIBXML2="v2.9.4"
CURRENT_LIBFTDI="1.3"
CURRENT_KIRIGAMI="6980f3d01a66a8989696ea66662ba249b583a422"
CURRENT_BREEZE_ICONS=""

# deal with all the command line arguments
if [[ $# -ne 2 && $# -ne 3 ]] ; then
	echo "wrong number of parameters, format:"
	echo "get-dep-lib.sh <platform> <install dir>"
	echo "get-dep-lib.sh single <install dir> <lib>"
	echo "get-dep-lib.sh singleAndroid <install dir> <lib>"
	echo "where"
	echo "<platform> is one of scripts, ios or android"
	echo "(the name of the directory where build.sh resides)"
	echo "<install dir> is the directory to clone in"
	echo "<lib> is the name to be cloned"
	exit -1
fi

PLATFORM=$1
INSTDIR=$2
if [ ! -d ${INSTDIR} ] ; then
	echo "creating dir"
	mkdir -p ${INSTDIR}
fi

# FIX FOR ANDROID,
if [ "$PLATFORM" == "singleAndroid" ] ; then
	CURRENT_LIBZIP="1.1.3"
	CURRENT_OPENSSL="OpenSSL_1_0_2o"
fi
# no curl and old libs (never version breaks)
# check whether to use curl or wget
if [ "`which curl`" == "" ] ; then
	CURL="wget "
else
	CURL="curl -O "
fi
BUILD_COMMON="libzip libgit2 googlemaps"
case ${PLATFORM} in
	scripts)
		BUILD="${BUILD_COMMON} hidapi libcurl libusb openssl libssh2"
		;;
	ios)
		BUILD="${BUILD_COMMON} libxslt"
		;;
	android)
		BUILD="${BUILD_COMMON} libxslt sqlite libxml2 openssl libftdi libusb"
		;;
	single)
		BUILD="$3"
		;;
	singleAndroid)
		BUILD="$3"
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

if [[ "$BUILD" = *"libcurl"* && ! -d libcurl ]]; then
	git clone https://github.com/curl/curl.git libcurl
	pushd libcurl
	git fetch origin
	if ! git checkout $CURRENT_LIBCURL ; then
		echo "Can't find the right tag in libcurl - giving up"
		exit -1
	fi
	popd
fi

if [[ "$BUILD" = *"libftdi"* && ! -d libftdi1 ]]; then
	${CURL} https://www.intra2net.com/en/developer/libftdi/download/libftdi1-${CURRENT_LIBFTDI}.tar.bz2
	tar -jxf libftdi1-${CURRENT_LIBFTDI}.tar.bz2
	mv libftdi1-${CURRENT_LIBFTDI} libftdi1
fi

if [[ "$BUILD" = *"libgit2"* && ! -d libgit2 ]]; then
	git clone https://github.com/libgit2/libgit2.git
	pushd libgit2
	git fetch origin
	if ! git checkout ${CURRENT_LIBGIT2} ; then
		echo "Can't find the right tag in libgit2 - giving up"
		exit -1
	fi
	popd
fi

if [[ "$BUILD" = *"libssh2"* && ! -d libssh2 ]]; then
	git clone https://github.com/libssh2/libssh2.git
	pushd libssh2
	git fetch origin
	if ! git checkout $CURRENT_LIBSSH2 ; then
		echo "Can't find the right tag in libssh2 - giving up"
		exit -1
	fi
	popd
fi

if [[ "$BUILD" = *"libusb"* && ! -d libusb ]]; then
	git clone https://github.com/libusb/libusb.git
	pushd libusb
	git fetch origin
	if ! git checkout $CURRENT_LIBUSB ; then
		echo "Can't find the right tag in libusb - giving up"
		exit -1
	fi
	popd
fi

if [[ "$BUILD" = *"libxml2"* && ! -d libxml2 ]]; then
	git clone https://github.com/GNOME/libxml2.git
	pushd libxml2
	git fetch origin
	if ! git checkout $CURRENT_LIBXML2 ; then
		echo "Can't find the right tag in libxml2 - giving up"
		exit -1
	fi
	popd
fi

if [[ "$BUILD" = *"libxslt"* && ! -d libxslt ]]; then
	git clone https://github.com/GNOME/libxslt.git
	pushd libxslt
	git fetch origin
	if ! git checkout $CURRENT_LIBXSLT ; then
		echo "Can't find the right tag in libxslt - giving up"
		exit -1
	fi
	popd
fi

if [[ "$BUILD" = *"libzip"* && ! -d libzip ]]; then
	${CURL} https://libzip.org/download/libzip-${CURRENT_LIBZIP}.tar.gz
	tar xzf libzip-${CURRENT_LIBZIP}.tar.gz
	mv libzip-${CURRENT_LIBZIP} libzip
fi

if [[ "$BUILD" = *"breeze-icons"* && ! -d breeze-icons ]]; then
	git clone https://github.com/kde/breeze-icons.git
	pushd breeze-icons
	git pull --rebase
	popd
fi

if [[ "$BUILD" = *"googlemaps"* && ! -d googlemaps ]]; then
	git clone https://github.com/Subsurface-divelog/googlemaps.git
	pushd googlemaps
	git fetch origin
	git checkout master
	git pull --rebase
	popd
fi

if [[ "$BUILD" = *"hidapi"* && ! -d hidapi ]]; then
	git clone https://github.com/signal11/hidapi.git
	pushd hidapi
	git fetch origin
	# there is no good tag, so just build master
#	if ! git checkout $CURRENT_HIDAPI ; then
#		echo "Can't find the right tag in hidapi - giving up"
#		exit -1
#	fi
	popd
fi

if [[ "$BUILD" = *"kirigami"* && ! -d kirigami ]]; then
	git clone -b master https://github.com/KDE/kirigami.git
	pushd kirigami
	if ! git checkout $CURRENT_KIRIGAMI ; then
		echo "Can't find the right tag in openssl - giving up"
		exit -1
	fi
	popd
fi

if [[ "$BUILD" = *"openssl"* && ! -d openssl ]]; then
	git clone https://github.com/openssl/openssl.git
	pushd openssl
	git fetch origin
	if ! git checkout $CURRENT_OPENSSL ; then
		echo "Can't find the right tag in openssl - giving up"
		exit -1
	fi
	popd
fi

if [[ "$BUILD" = *"sqlite"* && ! -d sqlite ]]; then
	${CURL} http://www.sqlite.org/2017/sqlite-autoconf-${CURRENT_SQLITE}.tar.gz
	tar -zxf sqlite-autoconf-${CURRENT_SQLITE}.tar.gz
	mv sqlite-autoconf-${CURRENT_SQLITE} sqlite
fi
