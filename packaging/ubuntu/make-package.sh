#!/bin/bash
# start from the directory above the combined subsurface & subsurface/libdivecomputer directory
if [[ $(pwd | grep "subsurface$") || ! -d subsurface || ! -d subsurface/libdivecomputer || ! -d subsurface/libgit2 ]] ; then
	echo "Please start this script from the folder ABOVE the subsurface source directory"
	echo "which includes libdivecomputer and libgit2 as subdirectories)."
	exit 1;
fi

GITVERSION=$(cd subsurface ; git describe | sed -e 's/-g.*$// ; s/^v//')
VERSION=$(echo $GITVERSION | sed -e 's/-/./')
echo "building Subsurface" $VERSION
if [[ -d subsurface_$VERSION ]]; then
	rm -rf subsurface_$VERSION.bak.prev
	mv subsurface_$VERSION.bak subsurface_$VERSION.bak.prev
	mv subsurface_$VERSION subsurface_$VERSION.bak
fi
mkdir subsurface_$VERSION
(cd subsurface ; tar cf - . ) | (cd subsurface_$VERSION ; tar xf - )
cd subsurface_$VERSION
rm -rf .git libdivecomputer/.git libgit2/.git marble-source/.git
echo $GITVERSION > .gitversion

dh_make --email dirk@hohndel.org -c gpl2 --createorig --single --yes -p subsurface_$VERSION
rm debian/*.ex debian/*.EX debian/README.*
cp ../subsurface/packaging/ubuntu/debian/control debian/control
cp ../subsurface/packaging/ubuntu/debian/copyright debian/copyright
cp ../subsurface/packaging/ubuntu/debian/rules debian/rules
cp ../subsurface/packaging/ubuntu/debian/source.lintian-overrides debian/source.lintian-overrides
# do something clever with changelog
mv debian/changelog debian/autocl
head -1 debian/autocl | sed -e 's/)/~trusty)/' -e 's/unstable/trusty/' > debian/changelog
cat ../subsurface/packaging/ubuntu/debian/changelog >> debian/changelog
tail -1 debian/autocl >> debian/changelog
rm -f debian/autocl

debuild -S 

# and now for utopic (precise can't build Qt5 based packages)
prev=trusty
rel=utopic
sed -i "s/${prev}/${rel}/g" debian/changelog
debuild -S
