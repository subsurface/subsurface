#!/bin/bash -e

# build Subsurface for Win32
#
# this file assumes that you have installed MXE on your system
# and installed a number of dependencies as well. Latest MXE
# version from git may not always work for Qt5 and Subsurface.
# Try to select an older release version like build-2016-10-12.
#
# cd ~/src
# git clone --branch build-2016-10-12 https://github.com/mxe/mxe
# cd mxe
#
# now create a file settings.mk
#---
# # This variable controls the number of compilation processes
# # within one package ("intra-package parallelism").
# # Set to higher value if you have a powerful machine.
# JOBS := 1
#
# # This variable controls the targets that will build.
# MXE_TARGETS :=  i686-w64-mingw32.shared
#
# # Uncomment the next line if you want to do debug builds later
# # qtbase_CONFIGURE_OPTS=-debug-and-release
#---
# (documenting this in comments is hard... you need to remove
# the first '#' of course)
#
# now you can start the build
#
# make libxml2 libxslt libusb1 libzip qt5 nsis
#
# After quite a while (depending on your machine anywhere from 15-20
# minutes to several hours) you should have a working MXE install in
# ~/src/mxe
#
# Now this script will come in:
#
# This makes some assumption about the filesystem layout so you
# can build linux and windows build out of the same sources
# Something like this:
#
# ~/src/mxe                    <- MXE git with Qt5, automake (see above)
#      /grantlee               <- Grantlee 5.0.0 sources from git
#      /libssh2                <- from git - v1.6 seems to work
#      /libcurl                <- from git - 7.42.1 seems to work - rename folder!
#      /subsurface             <- current subsurface git
#      /libdivecomputer        <- appropriate libdc/Subsurface-branch branch
#      /libgit2                <- libgit2 0.23.1 or similar
#
# ~/src/win32                  <- build directory
#
# then start this script from ~/src/win32
#
#  cd ~/src/win32
#  bash ../subsurface/packaging/windows/mxe-based-build.sh installer
#
# this should create the latest daily installer
#
# in order not to keep recompiling everything this script only compiles
# the other components if their directories are missing or if a "magic
# file" has been touched.
#
# so if you update one of the other libs do
#
# cd ~/src/win32
# touch build.<component>
# bash ../subsurface/packaging/windows/mxe-based-build.sh installer
#
# and that component gets rebuilt as well. E.g.
# touch build.libdivecomputer
# to rebuild libdivecomputer before you build Subsurface
#
# If you want to create a installer for the debug build call
#
#  bash ../subsurface/packaging/windows/mxe-based-build.sh debug installer
#
# please be aware of the fact that this installer will be a few 100MB large
#
#
# please send patches / additions to this file!
#

exec 1> >(tee ./winbuild.log) 2>&1

# for debugging
#trap "set +x; sleep 1; set -x" DEBUG

# this is run on a rather powerful machine - if you want less
# build parallelism, please change this variable
JOBS="-j4"

EXECDIR=`pwd`
BASEDIR=$(cd "$EXECDIR/.."; pwd)
BUILDDIR=$(cd "$EXECDIR"; pwd)
MXEDIR=${MXEDIR:-mxe}

echo $BUILDDIR

if [[ ! -d "$BASEDIR"/"$MXEDIR" ]] ; then
	echo "Please start this from the right directory "
	echo "usually a winbuild directory parallel to the mxe directory"
	exit 1
fi

echo "Building in $BUILDDIR ..."

export PATH="$BASEDIR"/"$MXEDIR"/usr/bin:$PATH:"$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared/qt5/bin/
export CXXFLAGS=-std=c++11

if [[ "$1" == "debug" ]] ; then
	RELEASE="Debug"
	DLL_SUFFIX="d"
	shift
	if [[ -f Release ]] ; then
		rm -rf *
	fi
	touch Debug
else
	RELEASE="Release"
	DLL_SUFFIX=""
	if [[ -f Debug ]] ; then
		rm -rf *
	fi
	touch Release
fi

# grantlee

cd "$BUILDDIR"
if [[ ! -d grantlee || -f build.grantlee ]] ; then
	rm -f build.grantlee
	mkdir -p grantlee
	cd grantlee
	i686-w64-mingw32.shared-cmake -DCMAKE_TOOLCHAIN_FILE="$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake \
		-DCMAKE_BUILD_TYPE=$RELEASE \
		-DBUILD_TESTS=OFF \
		"$BASEDIR"/grantlee

	make $JOBS
	make install
fi


# libssh2:

cd "$BUILDDIR"
if [[ ! -d libssh2 || -f build.libssh2 ]] ; then
	rm -f build.libssh2
	mkdir -p libssh2
	cd libssh2

	i686-w64-mingw32.shared-cmake -DCMAKE_TOOLCHAIN_FILE="$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake \
		-DCMAKE_BUILD_TYPE=$RELEASE \
		-DBUILD_EXAMPLES=OFF \
		-DBUILD_TESTING=OFF \
		-DBUILD_SHARED_LIBS=ON \
		"$BASEDIR"/libssh2
	make $JOBS
	make install
	# don't install your dlls in bin, please
	cp "$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared/bin/libssh2.dll "$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared/lib
fi


# libcurl

cd "$BUILDDIR"
if [[ ! -d libcurl || -f build.libcurl ]] ; then
	rm -f build.libcurl
	mkdir -p libcurl
	cd libcurl
	../../libcurl/configure --host=i686-w64-mingw32.shared \
		--prefix="$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared/ \
		--disable-ftp \
		--disable-ldap \
		--disable-ldaps \
		--disable-rtsp \
		--enable-proxy \
		--enable-dict \
		--disable-telnet \
		--disable-tftp \
		--disable-pop3 \
		--disable-imap \
		--disable-smb \
		--disable-smtp \
		--disable-gopher \
		--disable-manual \
		--with-libssh2="$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared/

	# now remove building the executable
	sed -i 's/SUBDIRS = lib src include/SUBDIRS = lib include/' Makefile

	make $JOBS
	make install
fi


# libgit2:

cd "$BUILDDIR"
if [[ ! -d libgit2 || -f build.libgit2 ]] ; then
	rm -f build.libgit2
	mkdir -p libgit2
	cd libgit2
	i686-w64-mingw32.shared-cmake -DCMAKE_TOOLCHAIN_FILE="$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake \
		-DBUILD_CLAR=OFF -DTHREADSAFE=ON \
		-DCMAKE_BUILD_TYPE=$RELEASE \
		-DDLLTOOL="$BASEDIR"/"$MXEDIR"/usr/bin/i686-w64-mingw32.shared-dlltool \
		"$BASEDIR"/libgit2
	make $JOBS
	make install
fi

# libdivecomputer
#
# this one is special because we want to make sure it's in sync
# with the Linux builds, but we don't want the autoconf files cluttering
# the original source directory... so the "$BASEDIR"/libdivecomputer is
# a local clone of the "real" libdivecomputer directory

cd "$BUILDDIR"
if [[ ! -d libdivecomputer || -f build.libdivecomputer ]] ; then
	rm -f build.libdivecomputer
	cd "$BASEDIR"/libdivecomputer
	git pull
	cd "$BUILDDIR"
	mkdir -p libdivecomputer
	cd libdivecomputer

	"$BASEDIR"/libdivecomputer/configure --host=i686-w64-mingw32.shared \
		--enable-shared \
		--prefix="$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared
	make $JOBS
	make install
else
	echo ""
	echo ""
	echo "WARNING!!!!"
	echo ""
	echo "libdivecoputer not rebuilt!!"
	echo ""
	echo ""
fi

###############
# finally, Subsurface

cd "$BUILDDIR"
echo "Starting Subsurface Build"

# things go weird if we don't create a new build directory... Subsurface
# suddenly gets linked against Qt5Guid.a etc...
rm -rf subsurface

# first copy the Qt plugins in place
QT_PLUGIN_DIRECTORIES="$BASEDIR/"$MXEDIR"/usr/i686-w64-mingw32.shared/qt5/plugins/iconengines \
$BASEDIR/"$MXEDIR"/usr/i686-w64-mingw32.shared/qt5/plugins/imageformats \
$BASEDIR/"$MXEDIR"/usr/i686-w64-mingw32.shared/qt5/plugins/platforms \
$BASEDIR/"$MXEDIR"/usr/i686-w64-mingw32.shared/qt5/plugins/geoservices \
$BASEDIR/"$MXEDIR"/usr/i686-w64-mingw32.shared/qt5/plugins/printsupport"

STAGING_DIR=$BUILDDIR/subsurface/staging
STAGING_TESTS_DIR=$BUILDDIR/subsurface/staging_tests

mkdir -p $STAGING_DIR/plugins
mkdir -p $STAGING_TESTS_DIR

for d in $QT_PLUGIN_DIRECTORIES
do
	mkdir -p $STAGING_DIR/plugins/$(basename $d)
	mkdir -p $STAGING_TESTS_DIR/$(basename $d)
	for f in $d/*
	do
		if [[ "$d" =~  geoservice ]] && [[ ! "$f" =~ esri ]] ; then
			continue
		fi
		if [[ "$RELEASE" == "Release" ]] && ([[ ! -f ${f//d.dll/.dll} || "$f" == "${f//d.dll/.dll}" ]]) ; then
			cp $f $STAGING_DIR/plugins/$(basename $d)
			cp $f $STAGING_TESTS_DIR/$(basename $d)
		elif [[ "$RELEASE" == "Debug" && ! -f ${f//.dll/d.dll} ]] ; then
			cp $f $STAGING_DIR/plugins/$(basename $d)
			cp $f $STAGING_TESTS_DIR/$(basename $d)
		fi
	done
done

# next we need the QML modules
QT_QML_MODULES="$BASEDIR/"$MXEDIR"/usr/i686-w64-mingw32.shared/qt5/qml/QtQuick.2 \
$BASEDIR/"$MXEDIR"/usr/i686-w64-mingw32.shared/qt5/qml/QtLocation \
$BASEDIR/"$MXEDIR"/usr/i686-w64-mingw32.shared/qt5/qml/QtPositioning"

mkdir -p $STAGING_DIR/qml

for d in $QT_QML_MODULES
do
	cp -a $d $STAGING_DIR/qml
done

# for some reason we aren't installing Qt5Xml.dll and Qt5Location.dll
# I need to figure out why and fix that, but for now just manually copy that as well
EXTRA_MANUAL_DEPENDENCIES="$BASEDIR/"$MXEDIR"/usr/i686-w64-mingw32.shared/qt5/bin/Qt5Xml$DLL_SUFFIX.dll \
$BASEDIR/"$MXEDIR"/usr/i686-w64-mingw32.shared/qt5/bin/Qt5Location$DLL_SUFFIX.dll"

for f in $EXTRA_MANUAL_DEPENDENCIES
do
    cp $f $STAGING_DIR
    cp $f $STAGING_TESTS_DIR
done

cd "$BUILDDIR"/subsurface

i686-w64-mingw32.shared-cmake -DCMAKE_TOOLCHAIN_FILE="$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake \
	-DCMAKE_PREFIX_PATH="$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared/qt5 \
	-DCMAKE_BUILD_TYPE=$RELEASE \
	-DQT_TRANSLATION_DIR="$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared/qt5/translations \
	-DMAKENSIS=i686-w64-mingw32.shared-makensis \
	-DLIBDIVECOMPUTER_INCLUDE_DIR="$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared/include \
	-DLIBDIVECOMPUTER_LIBRARIES="$BASEDIR"/"$MXEDIR"/usr/i686-w64-mingw32.shared/lib/libdivecomputer.dll.a \
	-DNO_MARBLE=ON -DMAKE_TESTS=OFF \
	"$BASEDIR"/subsurface

make $JOBS "$@"
