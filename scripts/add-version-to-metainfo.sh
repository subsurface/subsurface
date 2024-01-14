#!/bin/bash
#
# work either when building from git or when building from our hand
# crafted tar balls for OBS
# this will, however, fail for plain tar balls create via git archive

SCRIPT_DIR="$( cd "${BASH_SOURCE%/*}" ; pwd )"
VERSION=$(git describe --match "v[0-9]*" --abbrev=12) || VERSION=$(cat "$SCRIPT_DIR"/../.gitversion)
DATE=$(git log -1 --format="%ct" | xargs -I{} date -d @{} +%Y-%m-%d)
if [ "$DATE" = "" ] ; then
	DATE=$(cat "$SCRIPT_DIR"/../.gitdate)
fi
sed -e "s|<release version=\"\" date=\"\" />|<release version=\"$VERSION\" date=\"$DATE\" />|" metainfo/subsurface.metainfo.xml.in > metainfo/subsurface.metainfo.xml
