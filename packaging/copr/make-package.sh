#!/bin/bash -e
# start from the directory above the subsurface directory
#
# we need to build the google maps plugin which is not part of our sources, so let's make sure
# it is included in our source tar file
#
# a lot of this is redundant with the OBS code - but I'm trying to make both of these scripts reasonably
# complete so a user could simply run one of them and not the other - well, of course a user would have
# to make changes as they can't write to the OBS/COPR repos that I own... but this should be a great
# starting point.

if [[ ! -d subsurface || ! -d subsurface/libdivecomputer ]] ; then
	echo "Please start this script from the folder ABOVE the subsurface source directory"
	exit 1;
fi
if [[ ! -d googlemaps ]] ; then
	echo "Please make sure you have the current master of https://github.com/Subsurface/googlemaps"
	echo "checked out in parallel to the Subsurface directory"
	exit 1;
fi

TOPDIR=$(pwd)

# ensure that the libdivecomputer module is there and current
cd subsurface
git submodule init
git submodule update

GITVERSION=$(bash scripts/get-version 4)
GITDATE=$(git log -1 --format="%at" | xargs -I{} date -d @{} +%Y-%m-%d)
LIBDCREVISION=$(cd libdivecomputer ; git rev-parse --verify HEAD)
FOLDER="subsurface-$GITVERSION"

# hardcode to 1 to mark that this is a test build, not a full release
GITREVISION=1

cd -

echo "building Subsurface $GITVERSION with libdivecomputer $LIBDCREVISION"

# create the file system structure that the rpmbuild tools expect
rpmdev-setuptree

# in order for the tools to work as expected, let's run this from the HOME directory
cd "$HOME"

if [[ ! -d $FOLDER ]]; then
	mkdir "$FOLDER"
	echo "copying sources"

	(cd "$TOPDIR"/subsurface ; tar cf - . ) | (cd "$FOLDER" ; tar xf - )
	cd "$FOLDER"
	cp -a "$TOPDIR"/googlemaps .

	# make sure we only have the files we want (the builds should all be empty when running on GitHub)
	rm -rf .git libdivecomputer/.git googlemaps/.git build build-mobile libdivecomputer/build googlemaps/build
	echo "$GITVERSION" > .gitversion
	echo "$GITDATE" > .gitdate
	echo "$LIBDCREVISION" > libdivecomputer/revision
	cd ..
fi

if [[ ! -f rpmbuild/SOURCES/subsurface-$GITVERSION.orig.tar.xz ]] ; then
	tar ch "$FOLDER" | xz > "rpmbuild/SOURCES/subsurface-$GITVERSION.orig.tar.xz"
fi

# if the user wanted to post this automatically, do so
#
# because in the normal flow we should never have the need to rev the Release number in the spec
# file, this is hard coded to 1 - it's entirely possible that there will be use cases where we
# need to push additional versions... but it seems impossible to predict what exactly would drive
# those and how to automate them
if [[ "$1" = "current" ]] ; then
	# this is a release
	REPO="Subsurface"
	SUMMARY="official Fedora RPMs from the Subsurface project"
	DESCRIPTION="This is the current (or roughly 'weekly') Subsurface release build, provided by the Subsurface team, including our own custom libdivecomputer."
else
	REPO="Subsurface-test"
	SUMMARY="rolling build of the latest development version of Subsurface"
	DESCRIPTION="This is a 'daily' build of Subsurface, provided by the Subsurface team, based on the latest sources. The builds aren't always well tested."
fi
cd "$HOME"/rpmbuild
# shellcheck disable=SC2002
cat "$TOPDIR"/subsurface/packaging/copr/subsurface.spec | sed "s/%define latestVersion.*/%define latestVersion $GITVERSION/;s/DESCRIPTION/$DESCRIPTION/;s/SUMMARY/$SUMMARY/" > SPECS/subsurface.spec
rpmbuild --verbose -bs "$(pwd)/SPECS/subsurface.spec"
copr build --nowait $REPO "$(pwd)/SRPMS/subsurface-$GITVERSION"-1.fc*.src.rpm
