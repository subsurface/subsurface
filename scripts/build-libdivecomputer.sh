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

PLATFORM=$(uname)

# Use all cores, unless user set their own MAKEFLAGS
if [[ -z "${MAKEFLAGS+x}" ]]; then
	if [[ ${PLATFORM} == "Linux" ]]; then
		NUM_CORES="$(nproc)"
	elif [[ ${PLATFORM} == "Darwin" ]]; then
		NUM_CORES="$(sysctl -n hw.logicalcpu)"
	else
		NUM_CORES="4"
	fi
	echo "Using ${NUM_CORES} cores for the build"
	export MAKEFLAGS="-j${NUM_CORES}"
else
	echo "Using user defined MAKEFLAGS=${MAKEFLAGS}"
fi

make
make install
