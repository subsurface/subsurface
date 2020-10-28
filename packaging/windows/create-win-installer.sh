#!/bin/bash

# this is run inside the docker container
cd /__w

[ ! -d mxe ] || ln -s /win/mxe .

# prep the container
bash subsurface/.github/workflows/scripts/windows-container-prep.sh

# remove artifact from prior builds
rm mdbtools/include/mdbver.h

# build the installer
rm -rf win64
mkdir win64
cd win64

# build Subsurface and then smtk2ssrf
export MXEBUILDTYPE=x86_64-w64-mingw32.shared
bash -ex ../subsurface/packaging/windows/mxe-based-build.sh installer
mv subsurface/subsurface-*.exe /__w

bash -ex ../subsurface/packaging/windows/smtk2ssrf-mxe-build.sh -a -i

mv smtk-import/smtk2ssrf*.exe /__w

