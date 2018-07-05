# SPDX-License-Identifier: GPL-2.0

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f <foo>" at a time, but pass parallelism.
.NOTPARALLEL:


mobile:
	if test ! -d build-mobile; then (echo "error: please run build.sh before make"; exit -1;); fi
	cd build-mobile; LIBRARY_PATH=../install_root/lib make 
	cd build-mobile; LIBRARY_PATH=../install_root/lib make install

desktop:
	if test ! -d build; then (echo "error: please run build.sh before make"; exit -1;); fi
	cd build; LIBRARY_PATH=../install_root/lib make 
	cd build; LIBRARY_PATH=../install_root/lib make install

check:
	if test ! -d build; then (echo "error: please run build.sh before make"; exit -1;); fi
	cd build; LIBRARY_PATH=../install_root/lib make check


all: desktop mobile check
