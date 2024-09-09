#!/bin/bash -e
#
# resign the binaries in a DMG created on GitHub

# usage:
# resign.sh path-where-DMG-is-mounted temp-dir-where-output-happens version

croak() {
	echo "$0: $*" >&2
	echo "usage: $0 <path to mounted DMG> <path to working directory> <version number without leading v>" >&2
	exit 1
}

if [[ "$1" == "" || ! -d "$1" || ! -d "$1/Subsurface.app/Contents/MacOS" ]] ; then
	croak "$1 doesn't look like a mounted Subsurface DMG"
fi
if [[ "$2" == "" || ! -d "$2" ]] ; then
	mkdir -p "$2" || croak "can't create $2 as output directory"
	WORKING=$( cd "$2" && pwd )
fi
[[ "$3" == "" ]] && croak "missing a version argument"
VERSION="$3"

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd ../../.. && pwd )

DMGCREATE=create-dmg

mkdir "$WORKING"/staging
cd "$WORKING"
pushd staging
cp -a "$1/Subsurface.app" .


# remove anything codesign doesn't want us to sign
find Subsurface.app/Contents/Frameworks/ \( -name Headers -o -name \*.prl -o -name \*_debug \) -print0 | xargs -0 rm -rf

# codesign --deep doesn't find the shared libraries that are QML plugins
find Subsurface.app/Contents/Resources/qml -name \*.dylib -exec \
	codesign --options runtime --keychain "$HOME/Library/Keychains/login.keychain" -s "Developer ID Application: Dirk Hohndel" --deep --force {} \;

codesign --options runtime --keychain "$HOME/Library/Keychains/login.keychain" -s "Developer ID Application: Dirk Hohndel" --deep --force Subsurface.app

# ok, now the app is signed. let's notarize it
# first create a apple appropriate zip file;
# regular zip command isn't good enough, need to use "ditto"
ditto -c -k --sequesterRsrc --keepParent Subsurface.app "$WORKING/Subsurface-$VERSION.zip"

# this assumes that you have setup the notary tool and have the credentials stored
# in your keychain
xcrun notarytool submit "$WORKING/Subsurface-$VERSION.zip" --keychain-profile "notarytool-password" --wait
xcrun stapler staple Subsurface.app

popd

# it's not entirely clear if signing / stapling the DMG is required as well
# all I can say is that when I do both, it appears to work
$DMGCREATE --background "${DIR}/subsurface/packaging/macosx/DMG-Background.png" \
	--window-size 500 360 --icon-size 96 --volname "Subsurface-$VERSION" \
	--app-drop-link 380 205 \
	--volicon "${DIR}/subsurface/packaging/macosx/Subsurface.icns" \
	--icon "Subsurface" 110 205 "./Subsurface-$VERSION-CICD-release.dmg" ./staging

xcrun notarytool submit "./Subsurface-$VERSION-CICD-release.dmg" --keychain-profile "notarytool-password" --wait
xcrun stapler staple "Subsurface-$VERSION-CICD-release.dmg"
