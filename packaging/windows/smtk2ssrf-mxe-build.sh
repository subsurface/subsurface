#!/bin/bash -e

# This script is based in subsurface's packaging/windows/mxe-based-build.sh and
# works in the same fashion. Building needs to be done in a directory out of
# the source tree and, please, refer to said script for instructions on how to
# build.
#
# Subsurface *MUST* have been built before running this script, as the importer
# links against libsubsurface_corelib.a library.
# Although is possible to build the latest git version of the importer against
# whichever other version of subsurface, this should be avoided, and both
# versions, subsurface and smtk-import should be the same.
#
# Flags and options:
# -a (--auto):	    Mark the buils as "automatic". This assumes we are building
#		    in a automated environment (e.g. travis-ci) and doesn't try
#		    to change the git tag or repo. -t and -r flags are useless
#		    if -a has been set. Building in travis-ci is detected and so
#		    the flag is not necessary there.
# -i (--installer): Packs a windows installer. This should always be used.
# -t (--tag):       Defines which git version we want to build. Defaults to
#                   latest. E.g. -t v4.6.4
# -r (--repo):	    Set the repo you want to target for the build. If you are
#                   working on a github fork, you will usually have an "origin"
#		    and a "fork" repo, this enables choosing which repo you want
#		    to pull from.
# -b (--build):     Values: debug or release. Defines the build we want to do.
# -d (--dir):	    Specify a directory where a copy of the installer will be
#                   placed. This is a *must* if the script runs in a VM, and
#		    refers -usually- to a local dir mounted on the VM.
#
# Examples: (provided Subsurface has been previously cross built)
#
# For most pourposes, including travis builds, just
# smtk2ssrf-mxe-build.sh -i
# should be used.
#
# smtk2ssrf-mxe-build.sh -i -t master
# This will build a release installer of smtk2ssrf placed in a directory under
# the win-build directory where it has been launched, named smtk-import. It will
# build git latest master regardless of subsurface's cross built version.
#
# smtk2ssrf-mxe-build.sh -b debug
# This will build *just* a windows binary (no packing) of the latest master.
# Use with care, this flag *must* match subsurface's build one.
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
AUTO="${TRAVIS:-false}"

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
	echo -e "---> Building current git commit and Release type without installer $DEFAULT"
else
	while [ $# -gt 0 ]; do
		case $1 in
			-a|--auto)	AUTO="true"
					;;
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
	echo -e "$BLUE---> Subsurface tagged to:$LIGHT_GRAY ${SSRF_TAG:-latest}"
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

export PATH="$BASEDIR"/mxe/usr/bin:$PATH:"$BASEDIR"/mxe/usr/x86_64-w64-mingw32.shared/qt5/bin/
export CXXFLAGS=-std=c++17
export PKG_CONFIG_PATH_x86_64_w64_mingw32_static="$BASEDIR/mxe/usr/x86_64-w64-mingw32.static/lib/pkgconfig"
export PKG_CONFIG_PATH_x86_64_w64_mingw32_shared="$BASEDIR/mxe/usr/x86_64-w64-mingw32.shared/lib/pkgconfig"
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH_x86_64_w64_mingw32_static":"$PKG_CONFIG_PATH_x86_64_w64_mingw32_shared"

#
# Subsurface
#
if [ "$AUTO" = "false" ]; then
	cd "$BASEDIR/subsurface"
	git reset --hard master && echo -e "$BLUE---> Uncommited changes to Subsurface (if any) dropped$DEFAULT"
	git checkout master
	if [ ! -z "$GITREPO" ]; then
		git pull --rebase "$GITREPO" master || aborting "git pull failed, Subsurface not updated"
	else
		git pull --rebase || aborting "git pull failed, Subsurface not updated"
	fi
	echo -e "$BLUE---> Subsurface updated$DEFAULT"

	if [ "$SSRF_TAG" != "" ]; then
		git checkout "$SSRF_TAG" || aborting "Failed to checkout Subsurface's $SSRF_TAG."
	fi
fi

# Every thing is ok. Go on.
cd "$BUILDDIR"

# Blow up smtk-import binary dir and make it again, just to be extra-clean
rm -rf smtk-import && echo -e "$BLUE---> Deleted$LIGHT_GRAY $BUILDDIR/smtk-import folder$DEFAULT"
mkdir -p smtk-import && echo -e "$BLUE---> Created new$LIGHT_GRAY $BUILDDIR/smtk-import folder$DEFAULT"

# first copy the Qt plugins in place
QT_PLUGIN_DIRECTORIES="$BASEDIR/mxe/usr/x86_64-w64-mingw32.shared/qt5/plugins/iconengines \
$BASEDIR/mxe/usr/x86_64-w64-mingw32.shared/qt5/plugins/imageformats \
$BASEDIR/mxe/usr/x86_64-w64-mingw32.shared/qt5/plugins/styles \
$BASEDIR/mxe/usr/x86_64-w64-mingw32.shared/qt5/plugins/platforms"

# This comes from subsurface's mxe-based-build.sh. I'm not sure it is necessary
# but, well, it doesn't hurt.
EXTRA_MANUAL_DEPENDENCIES="$BASEDIR/mxe/usr/x86_64-w64-mingw32.shared/qt5/bin/Qt5Xml$DLL_SUFFIX.dll"

STAGING_DIR=$BUILDDIR/smtk-import/staging

mkdir -p "$STAGING_DIR"/plugins

for d in $QT_PLUGIN_DIRECTORIES
do
    [[ -d $d ]] && cp -a "$d" "$STAGING_DIR"/plugins
done

for f in $EXTRA_MANUAL_DEPENDENCIES
do
    [[ -f $f ]] && cp "$f" "$STAGING_DIR"
done

cd "$BUILDDIR"/smtk-import
mkdir -p staging

echo -e "$BLUE---> Building CMakeCache.txt$DEFAULT"
x86_64-w64-mingw32.shared-cmake \
	-DCMAKE_TOOLCHAIN_FILE="$BASEDIR"/mxe/usr/x86_64-w64-mingw32.shared/share/cmake/mxe-conf.cmake \
	-DPKG_CONFIG_EXECUTABLE="/usr/bin/pkg-config" \
	-DCMAKE_PREFIX_PATH="$BASEDIR"/mxe/usr/x86_64-w64-mingw32.shared/qt5 \
	-DCMAKE_BUILD_TYPE=$RELEASE \
	-DMAKENSIS=x86_64-w64-mingw32.shared-makensis \
	-DSSRF_CORELIB="$BUILDDIR"/subsurface/core/libsubsurface_corelib.a \
	"$BASEDIR"/subsurface/smtk-import

echo -e "$BLUE---> Building ...$DEFAULT"
if [ ! -z "$INSTALLER" ]; then
	make "$JOBS" "$INSTALLER"
else
	make "$JOBS"
fi

if [ ! -z "$DATADIR" ]; then
	echo -e "$BLUE---> Copying Smtk2ssrf installer to data folder$DEFAULT"
	cp -vf "$BUILDDIR"/smtk-import/smtk2ssrf-*.exe "$DATADIR"
fi

echo -e "$RED---> Building smtk2ssrf done$DEFAULT"
