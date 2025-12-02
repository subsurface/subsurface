#!/bin/bash

function croak () {
    echo $@
    exit 1
}

[ "$1" = "-clean" ] && CLEAN="1"

# verify CLI tools are installed and working
xcode-select -p > /dev/null 2>&1 || croak "Xcode CLI tools required"
clang --version > /dev/null 2>&1 || croak "Clang not working"

# verify Homebrew is installed
brew info > /dev/null 2>&1 || croak "Homebrew required"
brew install autoconf automake libtool pkg-config gettext confuse aciidoctor \
     cmake libgit2 libraw libftdi libmtp hidapi libusb libssh2 libzip \
     qtbase qtconnectivity qt5compat qtlocation qtpositioning qttools qtdeclarative \
        || croak "brew install failed"


# verify Qt 6 is installed and in the path
qtver=$(qmake -query QT_VERSION 2>/dev/null) || { echo "Qt not found in PATH"; exit 1; }
[[ "$(printf '%s\n' "6.8.3" "$qtver" | sort -V | head -n1)" == "6.8.3" ]] || { echo "Qt $qtver < 6.8.3"; exit 1; }

# check if this looks like Qt binaries (vs Homebrew or something else)
grep -q "$qtver" <(qmake -query QT_INSTALL_LIBS) || { echo "this doesn't look like it's finding a Qt binary distribution";  exit 1;}

# clone the repo
# WARNING -- this is cloning the macos-qt6 branch... will need to be updated once merged
[ ! -d subsurface ] && git clone -b dirkhh-macos-qt6 https://github.com/subsurface/subsurface

# if requested, clean the old build remnants
[ "$CLEAN" = "1" ] && rm -rf googlemaps install-root subsurface/build subsurface/libdivecomputer/build

# build the user manual
pushd subsurface/Documentation || { echo "can't cd to subsurface/Documentation"; exit 1; }
make doc || { echo "failed to create HTML files"; exit 1; }
popd

# build the dependencies
export PKG_CONFIG_LIBDIR=$(pwd)/install-root/lib/pkgconfig
bash -e -x ./subsurface/scripts/build.sh -build-deps-only -ftdi

# # build the dependencies
# export PKG_CONFIG_LIBDIR=$(pwd)/install-root/lib/pkgconfig
# bash -e -x ./subsurface/scripts/build.sh -build-deps-only -ftdi
#
# mv build.log build-deps.log

# build Subsurface with Qt6
bash -e -x ./subsurface/scripts/build.sh -desktop -build-with-qt6 -build-with-homebrew -ftdi -install-docs

# not sure why I need to do this again here, but without this second install of googlemaps
# I get no map in Subsurface
pushd googlemaps/build
make install
popd

cd subsurface/build
codesign --force --deep --sign - Subsurface.app

echo "Run Subsurface with 'open subsurface/build/Subsurface.app'"
echo "rebuild by simply calling 'make' in the subsurface/build directory. Do not run 'make install' in that directory"
