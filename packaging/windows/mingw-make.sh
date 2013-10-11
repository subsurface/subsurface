#!/bin/sh
#
# just a small shell script that is used to invoke make with the right
# parameters to cross compile a binary for Windows
#
# the paths work for the default mingw32 install on Fedora - adjust as
# necessary

# force recreation of the nsi file in order to get the correct version
# number
rm packaging/windows/subsurface.nsi > /dev/null 2>&1

export PATH=/usr/i686-w64-mingw32/sys-root/mingw/bin:$PATH
mingw32-qmake-qt4 CROSS_PATH=/usr/i686-w64-mingw32/sys-root/mingw
mingw32-make $@
