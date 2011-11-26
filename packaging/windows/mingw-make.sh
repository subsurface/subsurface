#!/bin/bash
#
# just a small shell script that is used to invoke make with the right
# parameters to cross compile a binary for Windows
#
# the paths work for the default mingw32 install on OpenSUSE - adjust as
# necessary

make CC=i686-w64-mingw32-gcc \
	PKGCONFIG=i686-w64-mingw32-pkg-config \
	PKG_CONFIG_PATH=/usr/i686-w64-mingw32/sys-root/i686-w64-mingw32/lib/pkgconfig/ \
	XML2CONFIG=/usr/i686-w64-mingw32/sys-root/mingw/bin/xml2-config NAME=subsurface.exe
