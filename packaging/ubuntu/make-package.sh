#!/bin/bash
# start from the directory above the combined subsurface & subsurface/libdivecomputer directory
if [[ $(pwd | grep "subsurface$") || ! -d subsurface || ! -d subsurface/libdivecomputer || ! -d subsurface/libgit2 ]] ; then
	echo "Please start this script from the folder ABOVE the subsurface source directory"
	echo "which includes libdivecomputer and libgit2 as subdirectories)."
	exit 1;
fi

GITVERSION=$(cd subsurface ; git describe | sed -e 's/-g.*$// ; s/^v//')
GITREVISION=$(echo $GITVERSION | sed -e 's/.*-// ; s/.*\..*//')
VERSION=$(echo $GITVERSION | sed -e 's/-/./')
LIBDCREVISION=$(cd subsurface/libdivecomputer ; git rev-parse --verify HEAD)

#
#
echo "building Subsurface" $VERSION "with libdivecomputer" $LIBDCREVISION
#
if [[ -d subsurface_$VERSION ]]; then
	rm -rf subsurface_$VERSION.bak.prev
	mv subsurface_$VERSION.bak subsurface_$VERSION.bak.prev
	mv subsurface_$VERSION subsurface_$VERSION.bak
fi
rm -f subsurfacedaily-$VERSION

mkdir subsurface_$VERSION
ln -s subsurface_$VERSION subsurfacedaily-$VERSION
#
#
echo "copying sources"
#
(cd subsurface ; tar cf - . ) | (cd subsurface_$VERSION ; tar xf - )
cd subsurface_$VERSION
rm -rf .git libdivecomputer/.git libgit2/.git marble-source/.git
echo $GITVERSION > .gitversion
echo $LIBDCREVISION > libdivecomputer/revision
# dh_make --email dirk@hohndel.org -c gpl2 --createorig --single --yes -p subsurface_$VERSION
# rm debian/*.ex debian/*.EX debian/README.*
#
#
echo "creating source tar file for OBS and Ununtu PPA"
#
(cd .. ; tar ch subsurfacedaily-$VERSION | xz > home:Subsurface-Divelog/Subsurface-daily/subsurface-$VERSION.orig.tar.xz) &
tar cf - . | xz > ../subsurface_$VERSION.orig.tar.xz
#
#
echo "preparint the debian directory"
#
export DEBEMAIL=dirk@hohndel.org
mkdir -p debian
cp -a packaging/ubuntu/debian .
cp ../debian.changelog debian/changelog
# do something clever with changelog
#mv debian/changelog debian/autocl
#head -1 debian/autocl | sed -e 's/)/~trusty)/' -e 's/unstable/trusty/' > debian/changelog
#cat ../subsurface/packaging/ubuntu/debian/changelog >> debian/changelog
#tail -1 debian/autocl >> debian/changelog
#rm -f debian/autocl

rev=0
while [ $rev -le "99" ]
do
rev=$(($rev+1))
	if [[ ! $(grep $VERSION-$rev debian/changelog) ]] ; then
		break
	fi
done
dch -v $VERSION-$rev~trusty -D trusty -M -m "next daily build"
mv ~/src/debian.changelog ~/src/debian.changelog.previous
cp debian/changelog ~/src/debian.changelog

debuild -S 

# and now for utopic (precise can't build Qt5 based packages)
prev=trusty
rel=utopic
sed -i "s/${prev}/${rel}/g" debian/changelog
debuild -S

# and now for precise
prev=utopic
rel=precise
sed -i "s/${prev}/${rel}/g" debian/changelog
cp debian/12.04.control debian/control
cp debian/12.04.rules debian/rules
debuild -S

cd ..

if [[ "$1x" = "postx" ]] ; then
	dput ppa:subsurface/subsurface-daily subsurface_$VERSION-$rev~*.changes
	cd home:Subsurface-Divelog/Subsurface-daily
	osc rm $(ls subsurface*.tar.xz | grep -v $VERSION)
	osc add subsurface-$VERSION.orig.tar.xz
	sed -i "s/%define gitVersion .*/%define gitVersion $GITREVISION/" subsurfacedaily.spec
	osc commit -m "next daily build"
fi
