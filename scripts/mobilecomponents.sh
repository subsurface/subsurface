#!/bin/bash
#
# if you run the build.sh script to build Subsurface you'll never need
# this, but if you build your binaries differently for some reason and
# you want to build Subsurface-mobile, running this from within the
# checked out source directory (not your build directory) should do the
# trick - you can also run this to update to the latest upstream when
# you don't want to rerun the whole build.sh script.

SRC=$(cd .. ; pwd)

if [ ! -d "$SRC/subsurface" ] || [ ! -d "mobile-widgets" ] || [ ! -d "core" ] ; then
	echo "please start this script from the Subsurface source directory (which needs to be named \"subsurface\")."
	exit 1
fi

# Callers can set KIRIGAMI_BUILDDIR and KIRIGAMI_INSTALL_PREFIX to control
# where the Kirigami build artifacts and installed files end up. This keeps
# platform-specific output out of the source tree and allows building for
# multiple targets without cleaning.
KIRIGAMI_BUILDDIR="${KIRIGAMI_BUILDDIR:-$SRC/kirigami-build}"
KIRIGAMI_INSTALL_PREFIX="${KIRIGAMI_INSTALL_PREFIX:-$SRC/install-root}"

# fetch/update the source checkouts
./scripts/get-dep-lib.sh single "$SRC"/subsurface/mobile-widgets/3rdparty kirigami
./scripts/get-dep-lib.sh single "$SRC"/subsurface/mobile-widgets/3rdparty breeze-icons
./scripts/get-dep-lib.sh single "$SRC"/subsurface/mobile-widgets/3rdparty extra-cmake-modules

# now install the ECM to keep things more contained, install into 3rdparty/ECM
# clear CMAKE_PREFIX_PATH so ECM doesn't pick up a cross-compiled Qt
# always start clean to avoid stale CMakeCache.txt when the source tree is
# mounted at a different path (e.g. inside a Docker container)
rm -rf "$SRC"/subsurface/mobile-widgets/3rdparty/ECM
mkdir -p "$SRC"/subsurface/mobile-widgets/3rdparty/ECM
cd "$SRC"/subsurface/mobile-widgets/3rdparty/ECM
CMAKE_PREFIX_PATH="" cmake -G Ninja -DSHARE_INSTALL_DIR=.. ../extra-cmake-modules
cmake --build . --target install

# add our patches to Kirigami
cd "$SRC"/subsurface/mobile-widgets/3rdparty
PATCHES=$(echo 00*.patch)
cd kirigami
git am --abort 2>/dev/null || true
for i in $PATCHES
do
	git am ../$i
done

# finally, build and install Kirigami
# any extra arguments (e.g. cross-compilation flags) are forwarded to cmake
if [[ ${PLATFORM} == "Linux" ]]; then
	NUM_CORES="$(nproc)"
elif [[ ${PLATFORM} == "Darwin" ]]; then
	NUM_CORES="$(sysctl -n hw.logicalcpu)"
else
	NUM_CORES="4"
fi

cmake -G Ninja -B "$KIRIGAMI_BUILDDIR" -DBUILD_SHARED_LIBS=ON \
	-DCMAKE_INSTALL_PREFIX="$KIRIGAMI_INSTALL_PREFIX" \
	-DECM_DIR="$SRC"/subsurface/mobile-widgets/3rdparty/ECM/cmake \
	-DUSE_DBUS=OFF "$@"
cmake --build "$KIRIGAMI_BUILDDIR" -j"${NUM_CORES}"
cmake --install "$KIRIGAMI_BUILDDIR"
