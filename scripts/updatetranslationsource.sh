#!/bin/bash
#
# translations have become much simpler with Qt 6 (after some birthing pains),
# and as far as I can tell only one major annoyance remains... now build paths
# are included in the locations - which is redundant, as those are derived from
# existing source files already included.

# The recommended process now is as follows:
# do a regular Subsurface desktop build with Qt 6
# assuming Subsurface sources are in .../src/subsurface:
# cd .../src
# bash ./subsurface/scripts/build.sh -build-with-qt6 -desktop
#
# with this the build directory will be .../src/subsurface/build -- this is used
# later in this code!
#
# then run this script before pushing changes to Transifex

if [[ ! -d translations || ! -f translations/subsurface_source.qm ]] ; then
	echo "Start from the build folder after successful desktop build"
	exit 1
fi

if [[ $(basename "$(pwd)") != "build" || $(basename "$(cd .. ; pwd)") != "subsurface" ]] ; then
	echo "The directory layout doesn't match my expectations, please consult the source of this script"
	exit 1
fi

PUSH=""
COMMIT=""

while [ $# -gt 0 ]; do
        arg="$1"
        case $arg in
                -push)
                        PUSH="1"
                        ;;
                -commit)
                        COMMIT="1"
                        ;;
                *)
                        echo "Unknown command line argument $arg"
                        echo "Usage: ${BASH_SOURCE[0]} [-push] [-commit]"
        esac
        shift
done

SRC=$(grep Subsurface_SOURCE_DIR CMakeCache.txt | cut -d= -f2)

pushd "$SRC" 2>/dev/null || ( echo "pushd $SRC failed" ; exit )

# let's make sure the tree is clean
if git status | grep -E "(Changes not staged for commit|Changes to be committed)" 2>/dev/null ; then
	echo "tree not clean"
	exit 1
fi

popd 2>/dev/null || ( echo "popd failed" ; exit )

make update_translations 2>&1 | tee translationsupdate.log

cd "$SRC" || ( echo "cd $SRC failed" ; exit )

for TSFILE in translations/*.ts ; do sed -i.bak '/..\/build/d' "$TSFILE" ; done

if git status | grep -E "(Changes not staged for commit|Changes to be committed)" 2>/dev/null ; then
	if [ "$COMMIT" = "1" ] && git status | grep -E "(Changes not staged for commit|Changes to be committed)" 2>/dev/null ; then
		# now add the new source strings to git and remove the rest of the files we created
		git add translations/subsurface_source.ts
		git commit -s -m "Update translation source strings"
		git reset --hard
	fi

	if [ "$PUSH" = "1" ] ; then
		# push sources to Transifex
		tx push -s
	fi
else
	echo "no changes to translation sources"
fi
