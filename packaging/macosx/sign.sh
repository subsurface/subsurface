#!/bin/bash -e
#
# just a simple helper to make it easier for me to sign our binaries

echo -- signing staging/Subsurface.app
cd staging

# remove the spurious DBus framework that we don't need
#rm -rf Subsurface.app/Contents/Frameworks/QtDBus.framework

# remove anything codesign doesn't want us to sign
find Subsurface.app/Contents/Frameworks/ \( -name Headers -o -name \*.prl -o -name \*_debug \) -print0 | xargs -0 rm -rf

# codesign --deep doesn't find the shared libraries that are QML plugins
while IFS= read -r -d '' dylib; do
	codesign --options runtime --keychain "$HOME/Library/Keychains/login.keychain" -s "Developer ID Application: Dirk Hohndel" --deep --force "$dylib"
done < <(find Subsurface.app/Contents/Resources/qml -name '*.dylib' -print0)

codesign --options runtime --keychain "$HOME/Library/Keychains/login.keychain" -s "Developer ID Application: Dirk Hohndel" --deep --force Subsurface.app
