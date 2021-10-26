#!/bin/bash
# This script is meant to run from src directory in the same fashion than
# subsurface/scripts/build.sh
#
# Flags:	-c (--cli) Build commandline version (no graphics)
#		-t (--tag) A git valid tag, commit, etc in subsurface tree
#		-j (--jobs) Desired build parallelism, integer.
#		-b (--build) Cmake build type, valid values Debug or Release
#		-y Assume "yes" in prompt. Useful in automated builds
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
AUTO="${TRAVIS:-false}"

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
		-c|--cli)	CLI="ON"
				shift;;
		-t|--tag)	SSRF_TAG="$2"
				shift 2;;
		-j|--jobs)	JOBS=-j"$2"
				shift 2;;
		-b|--build)	RELEASE="$2"
				shift 2;;
		-y)		AUTO="true"
				shift;;
		*)		aborting "Wrong parameter $1"
	esac
done

if [ "$AUTO" == "true" ]; then
	_proceed="y"
else
	printf "
	*****  WARNING  *****
	Please, note that this script will render your Subsurface binary unusable.
	So, if you are using the binary placed in build directory, you will need
	to rebuild it after running this script.

	Proceed? [y/n]\n"

	read -rs _proceed
fi
[[ $_proceed != "y" && $_proceed != "Y" ]] && exit 0

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
if ! pkg-config --exists glib-2.0; then
	aborting "Glib-2.0 not installed"
else
	echo "----> Glib-2.0 exists: $(pkg-config --print-provides glib-2.0)"
fi

# Mdbtools
#
# Check if mdbtools devel package is avaliable, if it is not, download
# and build it.
#
if pkg-config --exists libmdb; then
	echo "----> Mdbtools already installed: $(pkg-config --print-provides libmdb)"
else
	echo "----> Downloading/Updating mdbtools "
	if [ -d "$BASEDIR"/mdbtools ]; then
		cd "$BASEDIR"/mdbtools || aborting "Couldn't cd into $BASEDIR/mdbtools"
		git pull --rebase || aborting "Problem downloading/updating mdbtools"
	else
		git clone https://github.com/brianb/mdbtools.git "$BASEDIR"/mdbtools || \
			aborting "Problem downloading/updating mdbtools"
	fi

	echo "----> Building mdbtools ..."
	echo "----> This will display a lot of errors and warnings"
	cd "$BASEDIR"/mdbtools || aborting "Couldn't cd into $BASEDIR/mdbtools"
	autoreconf -i -f
	./configure --prefix "$INSTALL_ROOT" --disable-man --disable-gmdb2 >/dev/null
	make "$JOBS">/dev/null || aborting "Building mdbtools failed"
	make install
fi

# Build bare metal Subsurface.
# We are going to modify some of the building parameters of Subsurface so
# will get a copy of the cmake cache to restore them after building smtk2ssrf.
cd "$SSRF_PATH" || aborting "Couldn't cd into $SSRF_PATH"
echo "----> Saving a copy of $SSRF_PATH/build/CMakeCache.txt"
cp -vf "$SSRF_PATH/build/CMakeCache.txt" "$SSRF_PATH/build/CMakeCache.txt.bak"
if [ ! "$SSRF_TAG" == "" ]; then
	PREV_GIT="$(git branch --no-color 2> /dev/null | sed -e '/^[^*]/d')"
	PREV_GIT=${PREV_GIT##*\ }; PREV_GIT=${PREV_GIT%)}
	git checkout "$SSRF_TAG" || STATUS=1
fi

# abort if git checkout failed
if [ ! -z "$STATUS" ] && [ "$STATUS" -eq 1 ]; then
	mv -f "$SSRF_PATH/build/CMakeCache.txt.bak" "$SSRF_PATH/build/CMakeCache.txt"
	aborting "Couldn't checkout $SSRF_TAG. Is it correct?"
fi

cmake   -DBTSUPPORT=OFF \
	-DCMAKE_BUILD_TYPE="$RELEASE" \
	-DFORCE_LIBSSH=OFF \
	-DFTDISUPPORT=OFF \
	-DMAKE_TESTS=OFF \
	-DNO_DOCS=ON \
	-DNO_PRINTING=ON \
	-DNO_USERMANUAL=ON \
	-DSUBSURFACE_TARGET_EXECUTABLE=DesktopExecutable \
	build
cd build || aborting "Couldn't cd into $SSRF_PATH/build directory"
make clean
LIBRARY_PATH=$INSTALL_ROOT/lib make "$JOBS" || STATUS=1

# Restore initial state of subsurface building system:
echo "----> Restoring Subsurface tree state"
[[ ! -z $PREV_GIT ]] && echo "------> Restoring git branch to - $PREV_GIT -" && \
	git checkout "$PREV_GIT" >/dev/null
echo "------> Restoring cmake cache" && \
	mv -f "$SSRF_PATH/build/CMakeCache.txt.bak" "$SSRF_PATH/build/CMakeCache.txt"
cmake .
echo "----> Restored. Rebuild subsurface if needed"

# Abort if failed to build subsurface
[[ ! -z $STATUS ]] && [[ $STATUS -eq 1 ]] && aborting "Couldn't build Subsurface"

# We are done. Move on
#
echo "----> Building smtk2ssrf SmartTrak divelogs importer"

cd "$SSRF_PATH"/smtk-import || aborting "Couldnt cd into $SSRF_PATH/smtk-import"
mkdir -p build
cd build || aborting "Couldn't cd into $SSRF_PATH/smtk-import/build"
_CMAKE_OPTS=( "-DCMAKE_BUILD_TYPE=$RELEASE" "-DCOMMANDLINE=${CLI:-OFF}" )
[[ $AUTO == "true" ]] && _CMAKE_OPTS+=( "-DCMAKE_INSTALL_PREFIX=$INSTALL_ROOT" )
cmake "${_CMAKE_OPTS[@]}" .. || aborting "cmake incomplete"
make clean
LIBRARY_PATH=$INSTALL_ROOT/lib make "$JOBS" || aborting "Failed to build smtk2ssrf"

# Install on automatic builds
if [ "$AUTO" == "true" ]; then
	LIBRARY_PATH=$INSTALL_ROOT/lib make install
else
	printf "
	>> Building smtk2ssrf completed <<
	>> Executable placed in  %s/smtk-import/build <<
	>> To install system-wide, move there and run sudo make install <<\n" "$SSRF_PATH"
fi
