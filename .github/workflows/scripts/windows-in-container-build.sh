#!/bin/bash
# this gets executed inside the container when building a Windows
# installer on Travis
#
# working directory is assumed to be the directory including all the
# source directories (subsurface, googlemaps, grantlee, etc)
# in order to be compatible with the assumed layout in the MXE script, we
# need to create the secondary build directory

set -x
set -e

mkdir -p win32
cd win32

# build Subsurface and then smtk2ssrf
export MXEBUILDTYPE=x86_64-w64-mingw32.shared
bash -ex ../subsurface/packaging/windows/mxe-based-build.sh installer

# the strange two step move is in order to get predictable names to use
# in the publish step of the GitHub Action
mv subsurface/subsurface.exe* ${GITHUB_WORKSPACE}/
mv subsurface/subsurface-*.exe ${GITHUB_WORKSPACE}/subsurface-installer.exe

bash -ex ../subsurface/packaging/windows/smtk2ssrf-mxe-build.sh -a -i

# the strange two step move is in order to get predictable names to use
# in the publish step of the GitHub Action
mv smtk-import/smtk2ssrf.exe ${GITHUB_WORKSPACE}/
mv smtk-import/smtk2ssrf*.exe ${GITHUB_WORKSPACE}/smtk2ssrf-installer.exe
