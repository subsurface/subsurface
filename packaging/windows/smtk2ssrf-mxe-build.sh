#!/bin/bash -e

# This script is based in subsurface's packaging/windows/mxe-based-build.sh and
# works in the same fashion. Building needs to be done in a directory out of
# the source tree and, please, refer to said script for instructions on how to
# build.
#
# Subsurface *MUST* have been built before running that script, as the importer
# links against libsubsurface_corelib.a library.
# Although is possible to build the latest git version of the importer against
# whichever other version of subsurface, this should be avoided, and both
# versions, subsurface and smtk-import should be the same.
#
# Flags and options:
# -i (--installer): Packs a windows installer. This should always be used.
# -t (--tag):       Defines which git version we want to build. Defaults to
#                   latest. E.g. -t v4.6.4
# -b (--build):     Values: debug or release. Defines the build we want to do.
# -d (--dir):	    Specify a directory where a copy of the installer will be
#                   placed. This is a *must* if the script runs in a VM, and
#		    refers -usually- to a local dir mounted on the VM.
#
# Examples: (provided Subsurface has been previously cross built)
#
# smtk2ssrf-mxe-build.sh -i -t master
# This will build an release installer of smtk2ssrf placed in a directory under
# the win-build directory where it has been launched, named smtk-import. It will
# build git latest master regardless of subsurface's cross built version.
#
# smtk2ssrf-mxe-build.sh -b debug
# This will build *just* a windows binary (no packing) of the latest master.
#
# smtk2ssrf-mxe-build.sh -i -t v4.6.4 -b relase -d /mnt/data
# As I'm building in a fedora-25 docker VM, this should bring up a release
# installer of the v4.6.4 tag, and put a copy in my local mounted dir. In
# fact this *should* fail to build because of portability issues in v4.6.4.
#

exec 1> >(tee ./winbuild_smtk2ssrf.log) 2>&1
# for debugging
# trap "set +x; sleep 1; set -x" DEBUG

# Set some colors for pretty output
#
BLUE="\033[0;34m"
RED="\033[0;31m"
LIGHT_GRAY="\033[0;37m"
DEFAULT="\033[0m"

SSRF_TAG=""
RELEASE="Release"

# this is important, if we are building in a VM or if we want to get a copy
# of the installer elsewhere out of the building tree.
# In my case this is a mount point on the docker VM.
DATADIR=""

# Adjust desired build parallelism
JOBS="-j1"

EXECDIR=$(pwd)
BASEDIR=$(cd "$EXECDIR/.."; pwd)
BUILDDIR=$(cd "$EXECDIR"; pwd)

GITREPO=""

# Display an error message if we need to bail out
#
function aborting() {
	echo -e "$RED----> $1. Aborting.$DEFAULT"
	exit 1
}

echo -e "$BLUE-> $BUILDDIR$DEFAULT"

if [[ ! -d "$BASEDIR"/mxe ]] ; then
	echo -e "$RED--> Please start this from the right directory"
	echo -e "usually a winbuild directory parallel to the mxe directory $DEFAULT"
	exit 1
fi

echo -e "$BLUE---> Building in$LIGHT_GRAY $BUILDDIR ...$DEFAULT"

# check for arguments and set options
if [ $# -eq 0 ]; then
	echo -e "$BLUE---> No arguments given."
	echo -e "---> Building actual git commit and Release type without installer $DEFAULT"
else
	while [ $# -gt 0 ]; do
		case $1 in
			-t|--tag)	SSRF_TAG="$2"
					shift;;
			-i|--installer)	INSTALLER="installer"
					;;
			-b|--build)	RELEASE="$2"
					shift;;
			-d|--dir)	DATADIR="$2"
					shift;;
			-r|--repo)	GITREPO="$2"
					shift;;
		esac
		shift
	done
	echo -e "$BLUE---> Subsurface tagged to:$LIGHT_GRAY $SSRF_TAG"
	echo -e "$BLUE---> Building type:$LIGHT_GRAY $RELEASE"
	echo -e "$BLUE---> Installer set to:$LIGHT_GRAY $INSTALLER $DEFAULT"
fi
case "$RELEASE" in
	debug|Debug)		RELEASE=Debug
				DLL_SUFFIX="d"
				[[ -f Release ]] && rm -rf ./*
				touch Debug
				;;
	release|Release)	RELEASE=Release
				DLL_SUFFIX=""
				[[ -f Debug ]] && rm -rf ./*
				touch Release
				;;
esac

export PATH="$BASEDIR"/mxe/usr/bin:$PATH:"$BASEDIR"/mxe/usr/i686-w64-mingw32.shared/qt5/bin/
export CXXFLAGS=-std=c++11
export PKG_CONFIG_PATH_i686_w64_mingw32_static="$BASEDIR/mxe/usr/i686-w64-mingw32.static/lib/pkgconfig"
export PKG_CONFIG_PATH_i686_w64_mingw32_shared="$BASEDIR/mxe/usr/i686-w64-mingw32.shared/lib/pkgconfig"
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH_i686_w64_mingw32_static":"$PKG_CONFIG_PATH_i686_w64_mingw32_shared"

#
# mdbtools
#
if [ ! -f "$BASEDIR"/mxe/usr/i686-w64-mingw32.static/lib/libmdb.a ]; then
	echo -e "$BLUE---> Building mdbtools ... $DEFAULT "
	mkdir -p --verbose "$BASEDIR"/mxe/usr/i686-w64-mingw32.static/include
	mkdir -p --verbose "$BASEDIR"/mxe/usr/i686-w64-mingw32.static/lib
	cd "$BUILDDIR"
	[[ -d mdbtools ]] && rm -rf mdbtools
	mkdir -p mdbtools
	cd mdbtools
	if [ ! -f "$BASEDIR"/mdbtools/configure ] ; then
		( cd "$BASEDIR"/mdbtools
		autoreconf -v -f -i )
	fi
	"$BASEDIR"/mdbtools/configure  --host=i686-w64-mingw32.static \
			     --srcdir="$BASEDIR"/mdbtools \
			     --prefix="$BASEDIR"/mxe/usr/i686-w64-mingw32.static \
			     --enable-shared \
			     --disable-man \
			     --disable-gmdb2
	make $JOBS >/dev/null || aborting "Building mdbtools failed."
	make install
else
	echo -e "$BLUE---> Prebuilt mxe mdbtools ... $DEFAULT"
fi

# Subsurface
#
cd "$BASEDIR/subsurface"
git reset --hard master && echo -e "$BLUE---> Uncommited changes to Subsurface (if any) dropped$DEFAULT"
if [ ! -z "$GITREPO" ]; then
	git pull --rebase "$GITREPO" master || aborting "git pull failed, Subsurface not updated"
else
	git pull --rebase || aborting "git pull failed, Subsurface not updated"
fi
echo -e "$BLUE---> Subsurface updated$DEFAULT"

if [ "$SSRF_TAG" != "" ]; then
	git checkout "$SSRF_TAG" || aborting "Failed to checkout Subsurface's $SSRF_TAG."
fi

# Every thing is ok. Go on.
cd "$BUILDDIR"

# Blow up smtk-import binary dir and make it again, just to be extra-clean
rm -rf smtk-import && echo -e "$BLUE---> Deleted$LIGHT_GRAY $BUILDDIR/smtk-import folder$DEFAULT"
mkdir -p smtk-import && echo -e "$BLUE---> Created new$LIGHT_GRAY $BUILDDIR/smtk-import folder$DEFAULT"

# first copy the Qt plugins in place
QT_PLUGIN_DIRECTORIES="$BASEDIR/mxe/usr/i686-w64-mingw32.shared/qt5/plugins/iconengines \
$BASEDIR/mxe/usr/i686-w64-mingw32.shared/qt5/plugins/imageformats \
$BASEDIR/mxe/usr/i686-w64-mingw32.shared/qt5/plugins/platforms"

# This comes from subsurface's mxe-based-build.sh. I'm not sure it is necessary
# but, well, it doesn't hurt.
EXTRA_MANUAL_DEPENDENCIES="$BASEDIR/mxe/usr/i686-w64-mingw32.shared/qt5/bin/Qt5Xml$DLL_SUFFIX.dll"

STAGING_DIR=$BUILDDIR/smtk-import/staging

mkdir -p "$STAGING_DIR"/plugins

for d in $QT_PLUGIN_DIRECTORIES
do
    cp -a "$d" "$STAGING_DIR"/plugins
done

for f in $EXTRA_MANUAL_DEPENDENCIES
do
    cp "$f" "$STAGING_DIR"
done

# this is absolutely hackish, but necessary. Libmdb (built or prebuilt) is linked against
# shared glib-2.0, but once and again we are trying to link against static lib.
mv -vf "$BASEDIR"/mxe/usr/i686-w64-mingw32.static/lib/libglib-2.0.a "$BASEDIR"/mxe/usr/i686-w64-mingw32.static/lib/libglib-2.0.a.bak || \
	echo -e "$BLUE------> libglib-2.0.a had been moved in a previous run$DEFAULT"

cd "$BUILDDIR"/smtk-import
mkdir -p staging

echo -e "$BLUE---> Building CMakeCache.txt$DEFAULT"
cmake	-DCMAKE_TOOLCHAIN_FILE="$BASEDIR"/mxe/usr/i686-w64-mingw32.shared/share/cmake/mxe-conf.cmake \
	-DPKG_CONFIG_EXECUTABLE="/usr/bin/pkg-config" \
	-DCMAKE_PREFIX_PATH="$BASEDIR"/mxe/usr/i686-w64-mingw32.shared/qt5 \
	-DCMAKE_BUILD_TYPE=$RELEASE \
	-DMAKENSIS=i686-w64-mingw32.shared-makensis \
	-DSSRF_CORELIB="$BUILDDIR"/subsurface/core/libsubsurface_corelib.a \
	"$BASEDIR"/subsurface/smtk-import

echo -e "$BLUE---> Building ...$DEFAULT"
if [ ! -z "$INSTALLER" ]; then
	make "$JOBS" "$INSTALLER"
else
	make "$JOBS"
fi

# Undo previous hackery
echo -e "$BLUE---> Restoring system to initial state$DEFAULT"
mv -vf "$BASEDIR"/mxe/usr/i686-w64-mingw32.static/lib/libglib-2.0.a.bak "$BASEDIR"/mxe/usr/i686-w64-mingw32.static/lib/libglib-2.0.a

if [ ! -z "$DATADIR" ]; then
	echo -e "$BLUE---> Copying Smtk2ssrf installer to data folder$DEFAULT"
	cp -vf "$BUILDDIR"/smtk-import/smtk2ssrf-*.exe "$DATADIR"
fi

echo -e "$RED---> Building smtk2ssrf done$DEFAULT"
