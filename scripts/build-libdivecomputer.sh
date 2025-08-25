#!/bin/bash
#
# this should be run from the subsurface directory

if [ ! -d libdivecomputer/src ] ; then
	git submodule init
	git submodule update --recursive
fi

mkdir -p libdivecomputer/build
cd libdivecomputer/build

if [ ! -f ../configure ] ; then
	# this is not a typo
	# in some scenarios it appears that autoreconf doesn't copy the
	# ltmain.sh file; running it twice, however, fixes that problem
	autoreconf --install ..
	autoreconf --install ..
fi

../configure --disable-examples --prefix=$INSTALL_ROOT

# Use all cores, unless user set -j<n>
if [[ ${PLATFORM} == "Linux" ]]; then
	MAKEFLAGS=" -j$(nproc) ${MAKEFLAGS:-}"
elif [[ ${PLATFORM} == "Darwin" ]]; then
	MAKEFLAGS="-j$(sysctl -n hw.logicalcpu) ${MAKEFLAGS:-}"
else
	MAKEFLAGS="-j4 ${MAKEFLAGS:-}"
fi

make
make install
