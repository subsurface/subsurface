#!/bin/sh
#
# just a small shell script that is used to invoke make with the right
# parameters to cross compile a binary for Windows
#
# the paths work for the default mingw32 install on Fedora - adjust as
# necessary
# this assumes that local cross builds for libdivecomputer and libgit2
# are in ../libdivecomputer and ../libgit2

# force recreation of the nsi file and Subsurface version header file
# in order to get the correct version number
BASEDIR=$(dirname $0)
rm $BASEDIR/subsurface.nsi > /dev/null 2>&1
rm $BASEDIR/../../ssrf-version.h > /dev/null 2>&1

if [[ $1 == "Qt5-64" ]] ; then
	export PATH=/usr/x86_64-w64-mingw32/sys-root/mingw/bin:$PATH
else
	export PATH=/usr/i686-w64-mingw32/sys-root/mingw/bin:$PATH
fi
export objdump=mingw-objdump

if [[ $1 == "Qt5-64" ]] ; then
	shift
	mingw64-qmake-qt5 \
		CROSS_PATH=/usr/x86_64-w64-mingw32/sys-root/mingw \
		QMAKE_LRELEASE=/usr/x86_64-w64-mingw32/bin/qt5/lrelease \
		QMAKE_MOC=/usr/x86_64-w64-mingw32/bin/qt5/moc \
		QMAKE_UIC=/usr/x86_64-w64-mingw32/bin/qt5/uic \
		QMAKE_RCC=/usr/x86_64-w64-mingw32/bin/qt5/rcc \
		LIBMARBLEDEVEL=../marble \
		LIBGIT2DEVEL=../libgit2 CONFIG+=libgit21-api \
		QMAKE_LIBDIR+=../openssl \
		CONFIG+=win64target \
		CONFIG+=brokenQt532win \
		$BASEDIR/../../subsurface.pro

elif [[ $1 == "Qt5" ]] ; then
	shift
	mingw32-qmake-qt5 \
		CROSS_PATH=/usr/i686-w64-mingw32/sys-root/mingw \
		QMAKE_LRELEASE=/usr/i686-w64-mingw32/bin/qt5/lrelease \
		QMAKE_MOC=/usr/i686-w64-mingw32/bin/qt5/moc \
		QMAKE_UIC=/usr/i686-w64-mingw32/bin/qt5/uic \
		QMAKE_RCC=/usr/i686-w64-mingw32/bin/qt5/rcc \
		LIBDCDEVEL=../libdivecomputer \
		LIBMARBLEDEVEL=../marble \
		LIBGIT2DEVEL=../libgit2 CONFIG+=libgit21-api \
		$BASEDIR/../../subsurface.pro

elif [[ $1 == "Qt5debug" ]] ; then
	shift
	mingw32-qmake-qt5 \
		CROSS_PATH=/usr/i686-w64-mingw32/sys-root/mingw \
		QMAKE_LRELEASE=/usr/i686-w64-mingw32/bin/qt5/lrelease \
		QMAKE_MOC=/usr/i686-w64-mingw32/bin/qt5/moc \
		QMAKE_UIC=/usr/i686-w64-mingw32/bin/qt5/uic \
		QMAKE_RCC=/usr/i686-w64-mingw32/bin/qt5/rcc \
		QMAKE_CFLAGS_RELEASE='$$QMAKE_CFLAGS_DEBUG -O0 -g' \
		QMAKE_CXXFLAGS_RELEASE='$$QMAKE_CXXFLAGS_DEBUG -O0 -g' \
		LIBDCDEVEL=../libdivecomputer \
		LIBMARBLEDEVEL=../marble \
		LIBGIT2DEVEL=../libgit2 CONFIG+=libgit21-api \
		V=1 \
		$BASEDIR/../../subsurface.pro

else

	mingw32-qmake-qt4 \
		CROSS_PATH=/usr/i686-w64-mingw32/sys-root/mingw \
		LIBMARBLEDEVEL=../marble \
		LIBGIT2DEVEL=../libgit2 CONFIG+=libgit21-api \
		$BASEDIR/../../subsurface.pro
fi

mingw32-make $@
