#!/bin/bash

set -x
set -e

# this gets executed by Travis when building an installer for Windows
# it gets started from inside the subsurface directory
# with all the other projects downloaded and installed in parralel to
# subsurface; in order to be compatible with the assumed layout, we
# need to create the secondary build directory
cd ${TRAVIS_BUILD_DIR}/..
mkdir win32
ls -l
cd win32

# make sure we use the pre-installed version of MXE
export MXEBUILDTYPE=i686-w64-mingw32.shared

# the MXE build used in this VM doesn't support FTDI
bash -ex ${TRAVIS_BUILD_DIR}/packaging/windows/mxe-based-build.sh -noftdi installer
bash -ex ${TRAVIS_BUILD_DIR}/packaging/windows/smtk2ssrf-mxe-build.sh -i
