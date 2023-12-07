#!/bin/bash -e
# start from the directory above the subsurface directory
#
# we need to build the google maps plugin which is not part of our sources, so let's make sure
# it is included in our source tar file

if [[ $(pwd | grep "subsurface$") || ! -d subsurface || ! -d subsurface/libdivecomputer ]] ; then
	echo "Please start this script from the folder ABOVE the subsurface source directory"
	exit 1;
fi
if [[ ! -d googlemaps ]] ; then
	echo "Please make sure you have the current master of https://github.com/Subsurface/googlemaps"
	echo "checked out in parallel to the Subsurface directory"
	exit 1;
fi

# ensure that the libdivecomputer module is there and current
cd subsurface
git submodule init
git submodule update
cd -

GITVERSION=$(cd subsurface ; git describe --match "v[0-9]*" --abbrev=12 | sed -e 's/-g.*$// ; s/^v//')
GITREVISION=$(echo $GITVERSION | sed -e 's/.*-// ; s/.*\..*//')
VERSION=$(echo $GITVERSION | sed -e 's/-/./')
GITDATE=$(cd subsurface ; git log -1 --format="%at" | xargs -I{} date -d @{} +%Y-%m-%d)
LIBDCREVISION=$(cd subsurface/libdivecomputer ; git rev-parse --verify HEAD)

if [[ "$GITREVISION" = "" ]] ; then
	SUFFIX=".release"
	FOLDER="subsurface-$VERSION"
else
	SUFFIX=".daily"
	FOLDER="subsurfacedaily-$VERSION"
fi

echo "building Subsurface" $VERSION "with libdivecomputer" $LIBDCREVISION

# we put all of the files into the distrobuilds directory in order not to clutter the 'src' directory
mkdir -p distrobuilds
cd distrobuilds

if [[ ! -d $FOLDER ]]; then
	mkdir $FOLDER
	echo "copying sources"

	(cd ../subsurface ; tar cf - . ) | (cd $FOLDER ; tar xf - )
	cd $FOLDER
	cp -a ../../googlemaps .

	rm -rf .git libdivecomputer/.git googlemaps/.git build build-mobile libdivecomputer/build googlemaps/build
	echo $GITVERSION > .gitversion
	echo $GITDATE > .gitdate
	echo $LIBDCREVISION > libdivecomputer/revision

	if [[ "$GITREVISION" != "" ]] ; then
		(cd .. ; tar ch $FOLDER | xz > home:Subsurface-Divelog/Subsurface-daily/subsurface-$VERSION.orig.tar.xz) &
	else
		(cd .. ; tar ch $FOLDER | xz > home:Subsurface-Divelog/Subsurface/subsurface-$VERSION.orig.tar.xz) &
	fi
	cd ..
else
	echo "using existing source tree"
fi

# if the user wanted to post this automatically, do so
if [[ "$1" = "post" ]] ; then
	# daily vs. release
	if [[ "$GITREVISION" == "" ]] ; then
		# this is a release
		cd home:Subsurface-Divelog/Subsurface
		ls subsurface*.tar.xz | grep -v $VERSION 2>/dev/null && osc rm $(ls subsurface*.tar.xz | grep -v $VERSION)
		osc add subsurface-$VERSION.orig.tar.xz
		sed -i "s/%define latestVersion.*/%define latestVersion $VERSION/" subsurface.spec
		sed -i "s/%define gitVersion .*/%define gitVersion 0/" subsurface.spec
		osc commit -m "next release build"
	else
		cd home:Subsurface-Divelog/Subsurface-daily
		ls subsurface*.tar.xz | grep -v $VERSION 2>/dev/null && osc rm $(ls subsurface*.tar.xz | grep -v $VERSION)
		osc add subsurface-$VERSION.orig.tar.xz
		sed -i "s/%define latestVersion.*/%define latestVersion $VERSION/" subsurfacedaily.spec
		sed -i "s/%define gitVersion .*/%define gitVersion $GITREVISION/" subsurfacedaily.spec
		osc commit -m "next daily build"
	fi
fi
