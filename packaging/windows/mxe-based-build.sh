#!/bin/bash -e

# build Subsurface for Windows
#
# this file assumes that you have installed MXE on your system
# and installed a number of dependencies as well.
#
# cd ~/src
# git clone https://github.com/mxe/mxe
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
# MXE_TARGETS :=  x86_64-w64-mingw32.shared
#
# # Uncomment the next line if you want to do debug builds later
# # qtbase_CONFIGURE_OPTS=-debug-and-release
#---
# (documenting this in comments is hard... you need to remove
# the first '#' of course)
#
# now you can start the build
#
# make libxml2 libxslt libusb1 libzip libssh2 libftdi1 curl qt5 nsis
#
#     (if you intend to build Subsurface without user space FTDI support
#      you can drop libftdi1 from that list and start this script with
#      -noftdi )
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
#      /subsurface             <- current subsurface git
#      /googlemaps             <- Google Maps plugin for QtLocation from git
#      /hidapi                 <- HIDAPI library for libdivecomputer
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

if [[ "$1" == "-noftdi" ]] ; then
	shift
	FTDI="OFF"
else
	FTDI="ON"
fi

# this is run on a rather powerful machine - if you want less
# build parallelism, please change this variable
JOBS="-j4"

EXECDIR=`pwd`
BASEDIR=$(cd "$EXECDIR/.."; pwd)
BUILDDIR=$(cd "$EXECDIR"; pwd)
MXEDIR=${MXEDIR:-mxe}
MXEBUILDTYPE=${MXEBUILDTYPE:-x86_64-w64-mingw32.shared}

echo $BUILDDIR

if [[ ! -d "$BASEDIR"/"$MXEDIR" ]] ; then
	echo "Please start this from the right directory "
	echo "usually a winbuild directory parallel to the mxe directory"
	exit 1
fi

echo "Building in $BUILDDIR ..."

export PATH="$BASEDIR"/"$MXEDIR"/usr/bin:$PATH:"$BASEDIR"/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/bin/
export CXXFLAGS=-std=c++17

if [[ "$1" == "debug" ]] ; then
	RELEASE="Debug"
	RELEASE_MAIN="Debug"
	RELEASE_GM="debug"
	DLL_SUFFIX="d"
	shift
	if [[ -f Release ]] ; then
		rm -rf *
	fi
	touch Debug
else
	RELEASE="Release"
	RELEASE_MAIN="RelWithDebInfo"
	RELEASE_GM="release"
	DLL_SUFFIX=""
	if [[ -f Debug ]] ; then
		rm -rf *
	fi
	touch Release
fi



# libdivecomputer
# ensure the git submodule is present and the autotools are set up

cd "$BASEDIR"/subsurface
if [ ! -d libdivecomputer/src ] ; then
	git submodule init
	git submodule update --recursive
fi
if [ ! -f libdivecomputer/configure ] ; then
	cd libdivecomputer
	autoreconf --install
	autoreconf --install
fi

# if this is a 64bit build then build libmtp as that isn't available via MXE
# for 32bit builds the library currently fails to build, so support for
# MTP devices (right now just the Garmin Descent Mk2/Mk2i) is not available on 32bit Windows
if [ "$MXEBUILDTYPE" = "x86_64-w64-mingw32.shared" ] ; then
	cd "$BUILDDIR"
	if [[ ! -d libmtp || -f build.libmtp ]] ; then
		rm -f build.libmtp
		cd "$BASEDIR/libmtp"
		export NOCONFIGURE=1
		echo 'N' | bash autogen.sh
		cd "$BUILDDIR"
		mkdir -p libmtp
		cd libmtp
		"$BASEDIR"/libmtp/configure \
			CC="$MXEBUILDTYPE"-gcc \
			--host="$MXEBUILDTYPE" \
			--enable-shared \
			--prefix="$BASEDIR"/"$MXEDIR"/usr/"$MXEBUILDTYPE"
		make $JOBS
		make install
	fi
fi

cd "$BUILDDIR"
CURRENT_SHA=$(cd "$BASEDIR"/subsurface/libdivecomputer ; git describe)
PREVIOUS_SHA=$(cat "libdivecomputer.SHA" 2>/dev/null || echo)
if [ ! "$CURRENT_SHA" = "$PREVIOUS_SHA" ] || [ ! -d libdivecomputer ] || [ -f build.libdivecomputer ] ; then
	rm -f build.libdivecomputer
	mkdir -p libdivecomputer
	cd libdivecomputer

	"$BASEDIR"/subsurface/libdivecomputer/configure \
		CC="$MXEBUILDTYPE"-gcc \
		--host="$MXEBUILDTYPE" \
		--enable-shared \
		--prefix="$BASEDIR"/"$MXEDIR"/usr/"$MXEBUILDTYPE"
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

cd "$BUILDDIR"
if [[ ! -d googlemaps || -f build.googlemaps ]] ; then
	rm -f build.googlemaps
	cd "$BASEDIR"/googlemaps
	git pull
	cd "$BUILDDIR"
	mkdir -p googlemaps
	cd googlemaps
	"$BASEDIR"/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/bin/qmake PREFIX=$"$BASEDIR"/"$MXEDIR"/usr/"$MXEBUILDTYPE" "$BASEDIR"/googlemaps/googlemaps.pro
	make $JOBS $RELEASE_GM
	make "$RELEASE_GM"-install
fi

###############
# finally, Subsurface

cd "$BUILDDIR"
echo "Starting Subsurface Build"

# things go weird if we don't create a new build directory... Subsurface
# suddenly gets linked against Qt5Guid.a etc...
rm -rf subsurface

# first copy the Qt plugins in place
QT_PLUGIN_DIRECTORIES="$BASEDIR/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/plugins/iconengines \
$BASEDIR/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/plugins/imageformats \
$BASEDIR/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/plugins/platforms \
$BASEDIR/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/plugins/geoservices \
$BASEDIR/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/plugins/styles \
$BASEDIR/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/plugins/printsupport"

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
		if [[ "$d" =~  geoservice ]] && [[ ! "$f" =~ googlemaps ]] ; then
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
QT_QML_MODULES="$BASEDIR/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/qml/QtQuick.2 \
$BASEDIR/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/qml/QtLocation \
$BASEDIR/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/qml/QtPositioning"

mkdir -p $STAGING_DIR/qml

for d in $QT_QML_MODULES
do
	cp -a $d $STAGING_DIR/qml
done

# for some reason we aren't installing Qt5Xml.dll and Qt5Location.dll
# I need to figure out why and fix that, but for now just manually copy that as well
EXTRA_MANUAL_DEPENDENCIES="$BASEDIR/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/bin/Qt5Xml$DLL_SUFFIX.dll \
$BASEDIR/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/bin/Qt5Location$DLL_SUFFIX.dll \
$BASEDIR/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/bin/Qt5QmlWorkerScript$DLL_SUFFIX.dll"

for f in $EXTRA_MANUAL_DEPENDENCIES
do
    cp $f $STAGING_DIR
    cp $f $STAGING_TESTS_DIR
done

cd "$BUILDDIR"/subsurface

"$MXEBUILDTYPE"-cmake \
	-DCMAKE_PREFIX_PATH="$BASEDIR"/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5 \
	-DCMAKE_BUILD_TYPE=$RELEASE_MAIN \
	-DQT_TRANSLATION_DIR="$BASEDIR"/"$MXEDIR"/usr/"$MXEBUILDTYPE"/qt5/translations \
	-DMAKENSIS="$MXEBUILDTYPE"-makensis \
	-DLIBDIVECOMPUTER_INCLUDE_DIR="$BASEDIR"/"$MXEDIR"/usr/"$MXEBUILDTYPE"/include \
	-DLIBDIVECOMPUTER_LIBRARIES="$BASEDIR"/"$MXEDIR"/usr/"$MXEBUILDTYPE"/lib/libdivecomputer.dll.a \
	-DMAKE_TESTS=OFF \
	-DBTSUPPORT=ON -DBLESUPPORT=ON \
	-DFTDISUPPORT=$FTDI \
	-DLIBGIT2_FROM_PKGCONFIG=ON \
	"$BASEDIR"/subsurface

make $JOBS "$@"
