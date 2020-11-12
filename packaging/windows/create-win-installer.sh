#!/bin/bash

# this is run inside the docker container
cd /__w

[ ! -d mxe ] || ln -s /win/mxe .

# grab the version number
cd subsurface
VERSION=$(./scripts/get-version linux)
cd ..

# prep the container
bash subsurface/.github/workflows/scripts/windows-container-prep.sh

# remove artifact from prior builds
rm -f mdbtools/include/mdbver.h

# build the 64bit installer
rm -rf win64
mkdir win64
cd win64

# build Subsurface and then smtk2ssrf
export MXEBUILDTYPE=x86_64-w64-mingw32.shared
export PATH=/win/mxe/usr/bin:$PATH
bash -ex ../subsurface/packaging/windows/mxe-based-build.sh installer
mv subsurface/subsurface-$VERSION.exe /__w

bash -ex ../subsurface/packaging/windows/smtk2ssrf-mxe-build.sh -a -i

mv smtk-import/smtk2ssrf-$VERSION.exe /__w

if [ "$1" != "-64only" ] ; then
	# build the 32bit installer
	cd /__w
	rm -rf win32
	mkdir win32
	cd win32

	# build Subsurface and then smtk2ssrf
	export MXEBUILDTYPE=i686-w64-mingw32.shared
	bash -ex ../subsurface/packaging/windows/mxe-based-build.sh installer
	mv subsurface/subsurface-$VERSION.exe /__w/subsurface-32bit-$VERSION.exe
fi
