#!/bin/bash -e
# start from the directory above the subsurface directory
#
# we need to build the google maps plugin which is not part of our sources, so let's make sure
# it is included in our source tar file
#

if [[ ! -d subsurface || ! -d subsurface/libdivecomputer ]] ; then
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

GITVERSION=$(bash scripts/get-version 4)
GITDATE=$(git log -1 --format="%at" | xargs -I{} date -d @{} +%Y-%m-%d)
LIBDCREVISION=$(cd libdivecomputer ; git rev-parse --verify HEAD)
FOLDER="subsurface_$GITVERSION"

# hardcode to 1 to mark that this is a test build, not a full release
GITREVISION=1

cd -

export DEBSIGN_PROGRAM="gpg --passphrase-file passphrase_file.txt --batch --no-use-agent"
export DEBSIGN_KEYID="F58109E3"

if [[ "$GITVERSION" = "" ]] ; then
	SUFFIX=".release"
else
	SUFFIX=".daily"
fi

echo "building Subsurface $GITVERSION with libdivecomputer $LIBDCREVISION"

# we put all of the files into the distrobuilds directory in order not to clutter the 'src' directory
mkdir -p distrobuilds
cd distrobuilds

if [[ ! -d $FOLDER ]]; then
	mkdir "$FOLDER"
	echo "copying sources"

	(cd ../subsurface ; tar cf - . ) | (cd "$FOLDER" ; tar xf - )
	cd "$FOLDER";
	cp -a ../../googlemaps .

	rm -rf .git libdivecomputer/.git googlemaps/.git build build-mobile libdivecomputer/build googlemaps/build
	echo "$GITVERSION" > .gitversion
	echo "$GITDATE" > .gitdate
	echo "$LIBDCREVISION" > libdivecomputer/revision

	echo "creating source tar file for Ubuntu PPA"

	tar cf - . | xz > ../"$FOLDER.orig.tar.xz"
else
	echo "using existing source tree"
	cd "$FOLDER"
fi


echo "preparing the debian directory"

# this uses my (Dirk's) email address by default... simply set that variable in your
# environment prior to starting the script
[[ -z $DEBEMAIL ]] && export DEBEMAIL=dirk@subsurface-divelog.org

# copy over the debian files and allow maintaining a release and daily changelog files
rm -rf debian
mkdir -p debian
cp -a ../../subsurface/packaging/ubuntu/debian .

# start with the bundled dummy changelog, but use our last one if it exists
[[ -f ../changelog$SUFFIX ]] && cp "../changelog$SUFFIX" debian/changelog

# pick a new revision number
rev=0
while [ $rev -le "99" ]
do
rev=$((rev+1))
	if ! grep -q "$GITVERSION-$rev" debian/changelog ; then
		break
	fi
done

# we need to do this for each Ubuntu release we support - right now the oldest is 20.04/Focal
if [[ "$GITREVISION" = "" ]] ; then
	dch -v "$GITVERSION-$rev~focal" -D focal -M -m "Next release build - please check https://subsurface-divelog.org/category/news/ for details"
else
	dch -v "$GITVERSION-$rev~focal" -D focal -M -m "next rolling CICD release build"
fi

cp debian/changelog ../changelog$SUFFIX

debuild -S -d

# create builds for the newer Ubuntu releases that Launchpad supports
#
rel=focal
others="jammy mantic"
for next in $others
do
	sed -i "s/${rel}/${next}/g" debian/changelog
	debuild -S -d
	rel=$next
done
if [ -f debian/control.bak ] ; then
	mv debian/control.bak debian/control
fi

cd ..

if [[ "$1" = "current" ]] ; then
	# this is a current release
	dput ppa:subsurface/subsurface "$FOLDER-$rev"~*.changes
else
	dput ppa:subsurface/subsurface-daily "$FOLDER-$rev"~*.changes
fi
