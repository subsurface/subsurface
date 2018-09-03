#!/bin/sh
version=$(git describe --abbrev=12)
date=$(git log -1 --format="%at" | xargs -I{} date -d @{} +%Y-%m-%d)
sed -e "s|<release version=\"\" date=\"\" />|<release version=\"$version\" date=\"$date\" />|" -i appdata/subsurface.appdata.xml
