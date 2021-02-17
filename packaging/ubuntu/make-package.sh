#!/bin/bash -e
# start from the directory above the subsurface directory
#
# we need to build the google maps plugin which is not part of our sources, so let's make sure
# it is included in our source tar file
#
# also, for bionic we need newer versions of libgit2 than what ships with the
# distro, so let's just use libgit2 1.0 everywhere

if [[ $(pwd | grep "subsurface$") || ! -d subsurface || ! -d subsurface/libdivecomputer ]] ; then
	echo "Please start this script from the folder ABOVE the subsurface source directory"
	exit 1;
fi
if [[ ! -d googlemaps ]] ; then
	echo "Please make sure you have the current master of git://github.com/Subsurface/googlemaps"
	echo "checked out in parallel to the Subsurface directory"
	exit 1;
fi

if [[ ! -d libgit2 ]] ; then
	echo "Please make sure you have libgit2 1.0 from git://github.com/libgit2/libgit2"
	echo "checked out in parallel to the Subsurface directory"
	exit 1;
fi

# ensure that the libdivecomputer module is there and current
cd subsurface
git submodule init
git submodule update
cd -

GITVERSION=$(cd subsurface ; git describe --abbrev=12 | sed -e 's/-g.*$// ; s/^v//')
GITREVISION=$(echo $GITVERSION | sed -e 's/.*-// ; s/.*\..*//')
VERSION=$(echo $GITVERSION | sed -e 's/-/./')
GITDATE=$(cd subsurface ; git log -1 --format="%at" | xargs -I{} date -d @{} +%Y-%m-%d)
LIBDCREVISION=$(cd subsurface/libdivecomputer ; git rev-parse --verify HEAD)

if [[ "$GITVERSION" = "" ]] ; then
	SUFFIX=".release"
else
	SUFFIX=".daily"
fi

echo "building Subsurface" $VERSION "with libdivecomputer" $LIBDCREVISION

# we put all of the files into the distrobuilds directory in order not to clutter the 'src' directory
mkdir -p distrobuilds
cd distrobuilds

if [[ ! -d subsurface_$VERSION ]]; then
	mkdir subsurface_$VERSION
	echo "copying sources"

	(cd ../subsurface ; tar cf - . ) | (cd subsurface_$VERSION ; tar xf - )
	cd subsurface_$VERSION;
	cp -a ../../googlemaps .
	cp -a ../../libgit2 .

	# now make sure we have libgit2 1.0
	cd libgit2
	git fetch
	git checkout v1.0.0
	cd ..
	rm -rf .git libdivecomputer/.git googlemaps/.git build build-mobile libdivecomputer/build googlemaps/build libgit2/.git libgit2/build
	echo $GITVERSION > .gitversion
	echo $GITDATE > .gitdate
	echo $LIBDCREVISION > libdivecomputer/revision

	echo "creating source tar file for Ubuntu PPA"

	tar cf - . | xz > ../subsurface_$VERSION.orig.tar.xz
else
	echo "using existing source tree"
	cd subsurface_$VERSION
fi


echo "preparing the debian directory"

# this uses my (Dirk's) email address by default... simply set that variable in your
# environment prior to starting the script
test -z $DEBEMAIL && export DEBEMAIL=dirk@hohndel.org

# copy over the debian files and allow maintaining a release and daily changelog files
rm -rf debian
mkdir -p debian
cp -a ../../subsurface/packaging/ubuntu/debian .

# start with the bundled dummy changelog, but use our last one if it exists
test -f ../changelog$SUFFIX && cp ../changelog$SUFFIX debian/changelog

# pick a new revision number
rev=0
while [ $rev -le "99" ]
do
rev=$(($rev+1))
	if [[ ! $(grep $VERSION-$rev debian/changelog) ]] ; then
		break
	fi
done

# we need to do this for each Ubuntu release we support - right now the oldest is 18.04/Bionic
if [[ "$GITREVISION" = "" ]] ; then
	dch -v $VERSION-$rev~bionic -D bionic -M -m "Next release build - please check https://subsurface-divelog.org/category/news/ for details"
else
	dch -v $VERSION-$rev~bionic -D bionic -M -m "next daily build"
fi

cp debian/changelog ../changelog$SUFFIX

debuild -S -d

#create builds for the newer Ubuntu releases that Launchpad supports
rel=bionic
others="focal groovy"
for next in $others
do
	sed -i "s/${rel}/${next}/g" debian/changelog
	debuild -S -d
	rel=$next
done

cd ..

if [[ "$1" = "post" ]] ; then
	# daily vs. release
	if [[ "$GITREVISION" == "" ]] ; then
		# this is a release
		dput ppa:subsurface/subsurface subsurface_$VERSION-$rev~*.changes
	else
		dput ppa:subsurface/subsurface-daily subsurface_$VERSION-$rev~*.changes
	fi
fi
