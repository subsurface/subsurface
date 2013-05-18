#!/bin/sh
#
# just a small shell script that is used to invoke make with the right
# parameters to cross compile a binary for Windows
#
# the paths work for the default mingw32 install on OpenSUSE - adjust as
# necessary

# force recreation of the nsi file in order to get the correct version
# number
rm packaging/windows/subsurface.nsi

export PATH=/usr/i686-w64-mingw32/sys-root/mingw/bin:$PATH
make CC=i686-w64-mingw32-gcc CXX=i686-w64-mingw32-g++ \
	PKGCONFIG=i686-w64-mingw32-pkg-config \
	PKG_CONFIG_PATH=/usr/i686-w64-mingw32/sys-root/mingw/lib/pkgconfig/ \
	CROSS_PATH=/usr/i686-w64-mingw32/sys-root/mingw/ \
	XSLTCONFIG=/usr/i686-w64-mingw32/sys-root/mingw/bin/xslt-config \
	XML2CONFIG=/usr/i686-w64-mingw32/sys-root/mingw/bin/xml2-config install-cross-windows $@
