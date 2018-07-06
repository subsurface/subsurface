# SPDX-License-Identifier: GPL-2.0

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f <foo>" at a time, but pass parallelism.
.NOTPARALLEL:


mobile:
	if test ! -d build-mobile; then (echo "error: please run build.sh before make"; exit -1;); fi
	cd build-mobile; make 
	cd build-mobile; make install

desktop:
	if test ! -d build; then (echo "error: please run build.sh before make"; exit -1;); fi
	cd build; make 
	cd build; make install

check:
	if test ! -d build; then (echo "error: please run build.sh before make"; exit -1;); fi
	cd build; make check


all: desktop mobile check
