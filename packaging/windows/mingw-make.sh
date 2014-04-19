#!/bin/sh
#
# just a small shell script that is used to invoke make with the right
# parameters to cross compile a binary for Windows
#
# the paths work for the default mingw32 install on Fedora - adjust as
# necessary

# force recreation of the nsi file in order to get the correct version
# number
BASEDIR=$(dirname $0)
rm $BASEDIR/subsurface.nsi > /dev/null 2>&1

export PATH=/usr/i686-w64-mingw32/sys-root/mingw/bin:$PATH
mingw32-qmake-qt4 CROSS_PATH=/usr/i686-w64-mingw32/sys-root/mingw \
	LIBGIT2DEVEL=../libgit2 CONFIG+=libgit21-api $BASEDIR/../../subsurface.pro
mingw32-make $@
