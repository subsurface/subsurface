#!/bin/bash
# This script is meant to run from src directory in the same fashion than
# subsurface/scripts/build.sh
#
# Flags:	-t (--tag) A git valid tag, commit, etc in subsurface tree
#		-j (--jobs) Desired build parallelism, integer.
#		-b (--build) Cmake build type, valid values Debug or Release
# Examples:
#     $	./subsurface/scripts/build-smtk2ssrf.sh
#	No flags, will build a Release version, with -j4 parallelism in the
#	last set git tree position. This should be the usual use form.
#     $	./subsurface/scripts/build-smtk2ssrf.sh -j 2 -t v4.6.5 -b Debug
#	Will build a Debug version of smtk2ssrf from the tag v4.6.5 (proved it
#	exists) with -j2 parallelism.
#

# Default build parallelism
#
JOBS="-j4"

# Default subsurface git tag: none, assume last manually selected
# or set in build.sh
#
SSRF_TAG=""

# Default build type Release
#
RELEASE=Release

BASEDIR="$(pwd)"
INSTALL_ROOT="$BASEDIR"/install-root
SSRF_PATH="$BASEDIR"/subsurface

# Display an error message if we need to bail out
#
function aborting() {
	echo "----> $1. Aborting."
	exit 1
}

# check for arguments and set options if any
#
while [ $# -gt 0 ]; do
	case $1 in
		-t|--tag)	SSRF_TAG="$2"
				shift;;
		-j|--jobs)	JOBS=-j"$2"
				shift;;
		-b|--build)	RELEASE="$2"
				shift;;
	esac
	shift
done

echo ">> Building smtk2ssrf <<"

# Check if we are in a sane environment
#
[[ ! -d "$SSRF_PATH" ]] && aborting "Please start from the source root directory" || echo "--> Building from $BASEDIR"


# Ensure cmake and pkg-config have the corrects paths set
#
export CMAKE_PREFIX_PATH="$BASEDIR/install-root:$CMAKE_PREFIX_PATH"
export PKG_CONFIG_PATH="$BASEDIR/install-root/lib/pkgconfig"

# Check if we have glib-2.0 installed. This is a dependency for
# mdbtools.
#
pkg-config --exists glib-2.0
[[ $? -ne 0 ]] && aborting "Glib-2.0 not installed" || \
	echo "----> Glib-2.0 exists: $(pkg-config --print-provides glib-2.0)"

# Mdbtools
#
# Check if mdbtools devel package is avaliable, if it is not, download
# and build it.
#
pkg-config --exists libmdb
if [ $? -ne 0 ]; then
	echo "----> Downloading/Updating mdbtools "
	if [ -d "$BASEDIR"/mdbtools ]; then
		cd "$BASEDIR"/mdbtools || aborting "Couldn't cd into $BASEDIR/mdbtools"
		git pull --rebase
	else
		git clone https://github.com/brianb/mdbtools.git "$BASEDIR"/mdbtools
	fi
	[[ $? -ne 0 ]] && aborting "Problem downloading/updating mdbtools"

	echo "----> Building mdbtools ..."
	echo "----> This will display a lot of errors and warnings"
	cd "$BASEDIR"/mdbtools || aborting "Couldn't cd into $BASEDIR/mdbtools"
	autoreconf -i -f
	./configure --prefix "$INSTALL_ROOT" --disable-man --disable-gmdb2 >/dev/null
	make "$JOBS">/dev/null || aborting "Building mdbtools failed"
	make install
else
	echo "----> Mdbtools already installed: $(pkg-config --print-provides libmdb)"
fi

# We are done. Move on
# No need to update subsurface sources, as it has been previously
# done by build.sh
#
# Do we want to build a branch or commit other than the one used
# for build.sh?
#
if [ ! "$SSRF_TAG" == "" ]; then
	cd "$SSRF_PATH" || aborting "Couldn't cd into $SSRF_PATH"
	git checkout "$SSRF_TAG" || aborting "Couldn't checkout $SSRF_TAG. Is it correct?"
fi

echo "----> Building smtk2ssrf SmartTrak divelogs importer"

cd "$SSRF_PATH"/smtk-import || aborting "Couldnt cd into $SSRF_PATH/smtk-import"
mkdir -p build
cd build || aborting "Couldn't cd into $SSRF_PATH/smtk-import/build"

cmake -DCMAKE_BUILD_TYPE="$RELEASE" .. || aborting "Cmake incomplete"

make "$JOBS" || aborting "Failed to build smtk2ssrf"

echo ">> Building smtk2ssrf completed <<"
echo ">> Executable placed in  $SSRF_PATH/smtk-import/build <<"
echo ">> To install system-wide, move there and run sudo make install <<"
