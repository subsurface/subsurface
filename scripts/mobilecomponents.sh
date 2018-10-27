#!/bin/bash
#
# if you run the build.sh script to build Subsurface you'll never need
# this, but if you build your binaries differently for some reason and
# you want to build Subsurface-mobile, running this from within the
# checked out source directory (not your build directory) should do the
# trick - you can also run this to update to the latest upstream when
# you don't want to rerun the whole build.sh script.

SRC=$(cd .. ; pwd)

if [ ! -d "$SRC/subsurface" ] || [ ! -d "mobile-widgets" ] || [ ! -d "core" ] ; then
	echo "please start this script from the Subsurface source directory (which needs to be named \"subsurface\")."
	exit 1
fi

# now bring in the latest Kirigami mobile components plus a couple of icons that we need
# first, get the latest from upstream
# yes, this is a bit overkill as we clone a lot of stuff for just a few files, but this way
# we stop having to manually merge our code with upstream all the time
# as we get closer to shipping a production version we'll likely check out specific tags
# or SHAs from upstream
./scripts/get-dep-lib.sh single .. kirigami
./scripts/get-dep-lib.sh single .. breeze-icons

# now copy the components and a couple of icons into plae
MC=$SRC/subsurface/mobile-widgets/qml/kirigami
PMMC=../kirigami
BREEZE=../breeze-icons

rm -rf $MC
mkdir -p $MC/icons
cp -R $PMMC/* $MC/

cp $BREEZE/icons/actions/22/map-globe.svg $MC/icons
cp $BREEZE/icons/actions/24/dialog-cancel.svg $MC/icons
cp $BREEZE/icons/actions/24/distribute-horizontal-x.svg $MC/icons
cp $BREEZE/icons/actions/24/document-edit.svg $MC/icons
cp $BREEZE/icons/actions/24/document-save.svg $MC/icons
cp $BREEZE/icons/actions/24/go-next.svg $MC/icons
cp $BREEZE/icons/actions/24/go-previous.svg $MC/icons
cp $BREEZE/icons/actions/24/go-up.svg $MC/icons
cp $BREEZE/icons/actions/16/view-readermode.svg $MC/icons
cp $BREEZE/icons/actions/24/application-menu.svg $MC/icons
cp $BREEZE/icons/actions/22/gps.svg $MC/icons
cp $BREEZE/icons/actions/24/trash-empty.svg $MC/icons
cp $BREEZE/icons/actions/24/edit-copy.svg $MC/icons
cp $BREEZE/icons/actions/24/edit-paste.svg $MC/icons
cp $BREEZE/icons/actions/24/list-add.svg $MC/icons
cp $BREEZE/icons/actions/22/handle-left.svg $MC/icons
cp $BREEZE/icons/actions/22/handle-right.svg $MC/icons
cp $BREEZE/icons/actions/22/overflow-menu.svg $MC/icons

# kirigami now needs the breeze-icons internally as well
pushd $MC
ln -s $SRC/breeze-icons .

# do not show the action buttons when the keyboard is open
sed -i -e "s/visible: root.action/visible: root.action \&\& \!Qt.inputMethod.visible/g" src/controls/private/ActionButton.qml
sed -i -e "s/visible: root.leftAction/visible: root.leftAction \&\& \!Qt.inputMethod.visible/g" src/controls/private/ActionButton.qml
sed -i -e "s/visible: root.rightAction/visible: root.rightAction \&\& \!Qt.inputMethod.visible/g" src/controls/private/ActionButton.qml

# kirigami hack: passive notification hijacks area even after disabled.
# https://bugs.kde.org/show_bug.cgi?id=394204
sed -i -e "s/width: backgroundRect/enabled: root.enabled;    width: backgroundRect/g" src/controls/templates/private/PassiveNotification.qml

# another hack. Do not include the Kirigami resources (on static build). It causes
# double defined symbols in our setting. I would like a nicer fix for this
# issue, but failed to find one. For example, not adding the resource in
# our build causes the qrc file not to be generated. Manual generation
# of the resource file (using rcc) introduces the double symbols again. 
# so it seems some Kirigami weirdness (but their staticcmake example compiles
# correctly).
sed -i -e "s/#include <qrc_kirigami.cpp>/\/\/#include <qrc_kirigami.cpp>/" src/kirigamiplugin.cpp

# next hack - we want to use the topContent in the GlobalDrawer for our
# own image / logo / text (in part because the logo display got broken, in
# part because we want a second line of text to show the account ID
# for this to work we need to remove the margin that the GlobalDrawer
# forces around the topContent
patch -p0 < $SRC/subsurface/scripts/kirigami.diff

popd

echo org.kde.plasma.kirigami synced from upstream
