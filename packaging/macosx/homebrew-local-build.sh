#!/bin/bash

# This script will help to setup a local build environment on macOS using Homebrew
# dependencies, including Qt 6 from Homebrew
# It requires the Xcode command line tools and Homebrew itself to be installed
#
# Please note that as long as you have an Apple Developer account at all, you can
# install Xcode command line tools without being able to log into the AppStore, which
# can be extremely hard or even impossible from a macOS virtual machine.
#
# Currently this uses whatever Qt version Homebrew includes - I am guessing that
# when Homebrew switches to a newer version of Qt there's a fair chance that this will
# break and we may have to pin the version used for a while.

function croak () {
    echo $@
    exit 1
}

BRANCH="master"
# deal with the command line arguments
while [[ $# -gt 0 ]] ; do
	arg="$1"
	case $arg in
        -clean)
            # remove existing build artifacts and start fresh
            CLEAN="1"
            ;;
        -branch)
            # run against a different branch instead of master
            shift
            BRANCH="$1"
            ;;
        *)
            echo "Unknown command line argument $arg"
            echo "Usage: $0 [-clean] [-branch <branch>]"
            exit 1
            ;;
    esac
    shift
done

# verify CLI tools are installed and working
xcode-select -p > /dev/null 2>&1 || croak "Xcode CLI tools required"
clang --version > /dev/null 2>&1 || croak "Clang not working"

# verify Homebrew is installed
brew info > /dev/null 2>&1 || croak "Homebrew required"
brew install autoconf automake libtool pkg-config gettext confuse asciidoctor \
     cmake libgit2 libraw libftdi libmtp hidapi libusb libssh2 libzip \
     qtbase qtconnectivity qt5compat qtlocation qtpositioning qttools qtdeclarative \
        || croak "brew install failed"


# verify Qt 6 is installed and in the path
qtver=$(qmake -query QT_VERSION 2>/dev/null) || { echo "Qt not found in PATH"; exit 1; }
[[ "$(printf '%s\n' "6.8.3" "$qtver" | sort -V | head -n1)" == "6.8.3" ]] || { echo "Qt $qtver < 6.8.3"; exit 1; }

# clone the repo
[ ! -d subsurface ] && git clone -b "$BRANCH" https://github.com/subsurface/subsurface

# if requested, clean the old build remnants
[ "$CLEAN" = "1" ] && rm -rf googlemaps install-root subsurface/build subsurface/libdivecomputer/build

# build the user manual
pushd subsurface/Documentation || { echo "can't cd to subsurface/Documentation"; exit 1; }
make doc || { echo "failed to create HTML files"; exit 1; }
popd

# build Subsurface with Qt6
bash -e -x ./subsurface/scripts/build.sh -desktop -build-with-qt6 -build-with-homebrew -ftdi -install-docs

# now move the user manual into place
DATADIR="$(pwd)/subsurface/build/Subsurface.app/Contents/Resources/share/Documentation"
mkdir -p "$DATADIR"
pushd subsurface/Documentation/output
cp -a user-manual*.html images "$DATADIR"
popd

# not sure why I need to do this again here, but without this second install of googlemaps
# I get no map in Subsurface
pushd googlemaps/build
make install
popd

cd subsurface/build
codesign --force --deep --sign - Subsurface.app

echo "Run Subsurface with 'open subsurface/build/Subsurface.app'"
echo "rebuild by simply calling 'make' in the subsurface/build directory. Do not run 'make install' in that directory"
