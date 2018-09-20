#!/bin/bash
#
# work either when building from git or when building from our hand
# crafted tar balls for OBS
# this will, however, fail for plain tar balls create via git archive

SCRIPT_DIR="$( cd "${BASH_SOURCE%/*}" ; pwd )"
version=$(git describe --abbrev=12) || version=$(cat "$SCRIPT_DIR"/../.gitversion)
date=$(git log -1 --format="%ct" | xargs -I{} date -d @{} +%Y-%m-%d) || date=$(cat "$SCRIPT_DIR"/../.gitdate)
sed -e "s|<release version=\"\" date=\"\" />|<release version=\"$version\" date=\"$date\" />|" appdata/subsurface.appdata.xml.in > appdata/subsurface.appdata.xml
