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

# completely changing the Kirigami build, going down the path that the
# Kirigami developers favor, which is to install Kirigami and Breeze in
# a 3rdparty folder within our sources
./scripts/get-dep-lib.sh single "$SRC"/subsurface/mobile-widgets/3rdparty kirigami
./scripts/get-dep-lib.sh single "$SRC"/subsurface/mobile-widgets/3rdparty breeze-icons
./scripts/get-dep-lib.sh single "$SRC"/subsurface/mobile-widgets/3rdparty extra-cmake-modules

# now install the ECM to keep things more contained, install into 3rdparty/ECM
mkdir -p "$SRC"/subsurface/mobile-widgets/3rdparty/ECM
cd "$SRC"/subsurface/mobile-widgets/3rdparty/ECM
cmake -DSHARE_INSTALL_DIR=.. ../extra-cmake-modules
make install

# finally, add our patches to Kirigami
cd "$SRC"/subsurface/mobile-widgets/3rdparty
PATCHES=$(echo 00*.patch)
cd kirigami
for i in $PATCHES
do
	git am ../$i
done
