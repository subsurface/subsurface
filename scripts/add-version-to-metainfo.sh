#!/bin/bash
#
# work either when building from git or when building from our hand
# crafted tar balls for OBS
# this will, however, fail for plain tar balls created via git archive

SCRIPT_DIR="$( cd "${BASH_SOURCE%/*}" ; pwd )"

if [ -f "$SCRIPT_DIR"/../latest-subsurface-buildnumber ] ; then
	VERSION=$("$SCRIPT_DIR"/get-version.sh)
else
	sha=$(git log -1 --format="%H" 2>/dev/null)
	if [ "$sha" != "" ] ; then
		VERSION=$("$SCRIPT_DIR"/get-version-for-changeset.sh "$sha")
	else
		VERSION="unknown"
	fi
fi

if [ -f "$SCRIPT_DIR"/../.gitdate ] ; then
	DATE=$(cat "$SCRIPT_DIR"/../.gitdate)
else
	DATE=$(git log -1 --format="%ct" | xargs -I{} date -d @{} +%Y-%m-%d 2>/dev/null)
fi

sed -e "s|<release version=\"\" date=\"\" />|<release version=\"$VERSION\" date=\"$DATE\" />|" metainfo/subsurface.metainfo.xml.in > metainfo/subsurface.metainfo.xml
