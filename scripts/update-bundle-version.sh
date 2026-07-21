#!/bin/bash
# Update the iOS app bundle's Info.plist version strings from git.
# Called as a POST_BUILD step so the bundle stays in sync with git even when CMake isn't re-run.

set -e

PLIST="$1"
if [ -z "$PLIST" ] || [ ! -f "$PLIST" ]; then
	echo "update-bundle-version.sh: plist not found: $PLIST" >&2
	exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
V3="$(bash "$SCRIPT_DIR/get-version.sh" 3)"
V4="$(bash "$SCRIPT_DIR/get-version.sh" 4)"

/usr/libexec/PlistBuddy \
	-c "Set :CFBundleShortVersionString $V3" \
	-c "Set :CFBundleVersion $V4" \
	"$PLIST"
