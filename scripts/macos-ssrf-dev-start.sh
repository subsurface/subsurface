#!/bin/bash

# verify CLI tools are installed and working
xcode-select -p > /dev/null 2>&1 || { echo "Xcode CLI tools required"; exit 1; }
clang --version > /dev/null 2>&1 || { echo "Clang not working"; exit 1; }

# verify Qt 6 is installed and in the path
qtver=$(qmake -query QT_VERSION 2>/dev/null) || { echo "Qt not found in PATH"; exit 1; }
[[ "$(printf '%s\n' "6.8.3" "$qtver" | sort -V | head -n1)" == "6.8.3" ]] || { echo "Qt $qtver < 6.8.3"; exit 1; }

# check if this looks like Qt binaries (vs Homebrew or something else)
grep -q "$qtver" <(qmake -query QT_INSTALL_LIBS) || { echo "this doesn't look like it's finding a Qt binary distribution";  exit 1;}

# check for asciidoctor
hash asciidoctor >&/dev/null || { echo "missing asciidoctor - install from Homebrew"; exit 1; }

# clone the repo
# WARNING -- this is cloning the macos-qt6 branch... will need to be updated once merged
git clone -b dirkhh-macos-qt6 https://github.com/subsurface/subsurface

# build the user manual
pushd subsurface/Documentation || { echo "can't cd to subsurface/Documentation"; exit 1; }
make doc || { echo "failed to create HTML files"; exit 1; }
popd

# build the dependencies
export PKG_CONFIG_LIBDIR=$(pwd)/install-root/lib/pkgconfig
bash -e -x ./subsurface/scripts/build.sh -build-deps-only -ftdi

mv build.log build-deps.log

# build Subsurface with Qt6
bash -e -x ./subsurface/scripts/build.sh -desktop -build-with-qt6 -ftdi -install-docs

cd subsurface/build
codesign --force --deep --sign - Subsurface.app

