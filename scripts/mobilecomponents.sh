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

if [ "$1" = "-nopull" ] ; then
	NOPULL=1
fi

# now bring in the latest Kirigami mobile components plus a couple of icons that we need
# first, get the latest from upstream
# yes, this is a bit overkill as we clone a lot of stuff for just a few files, but this way
# we stop having to manually merge our code with upstream all the time
# as we get closer to shipping a production version we'll likely check out specific tags
# or SHAs from upstream
cd $SRC
if [ ! -d kirigami ] ; then
	git clone -b master https://github.com/KDE/kirigami.git
fi
if [ "$NOPULL" = "" ] ; then
	pushd kirigami
	git checkout master
	git pull origin master
	# if we want to pin a specific Kirigami version, we can do this here
	git checkout 162b877ad4b03d4843e21e7e4674f5b74c7688fe
	popd
fi
if [ ! -d breeze-icons ] ; then
	git clone git://anongit.kde.org/breeze-icons
fi
if [ "$NOPULL" = "" ] ; then
	pushd breeze-icons
	git pull
	popd
fi

# now copy the components and a couple of icons into plae
MC=$SRC/subsurface/mobile-widgets/qml/kirigami
PMMC=kirigami
BREEZE=breeze-icons

rm -rf $MC
mkdir -p $MC/icons
cp -R $PMMC/* $MC/

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
cp $BREEZE/icons/actions/24/list-add.svg $MC/icons
cp $BREEZE/icons/actions/22/handle-left.svg $MC/icons
cp $BREEZE/icons/actions/22/handle-right.svg $MC/icons
cp $BREEZE/icons/actions/22/overflow-menu.svg $MC/icons

# kirigami now needs the breeze-icons internally as well
pushd $MC
ln -s $SRC/$BREEZE .
sed -i -e "s/visible: root.action/visible: root.action \&\& \!Qt.inputMethod.visible/g" src/controls/private/ActionButton.qml
sed -i -e "s/visible: root.leftAction/visible: root.leftAction \&\& \!Qt.inputMethod.visible/g" src/controls/private/ActionButton.qml
sed -i -e "s/visible: root.rightAction/visible: root.rightAction \&\& \!Qt.inputMethod.visible/g" src/controls/private/ActionButton.qml
popd

echo org.kde.plasma.kirigami synced from upstream
