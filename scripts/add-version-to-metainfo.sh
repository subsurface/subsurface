#!/bin/bash
#
# work either when building from git or when building from our hand
# crafted tar balls for OBS
# this will, however, fail for plain tar balls created via git archive

# the script is run as "sh scripts/add-version-to-metainfo.sh", where sh may be
# dash and BASH_SOURCE is not set, so locate it through $0
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)" || exit 1

if [ -f "$SCRIPT_DIR"/../latest-subsurface-buildnumber ] ; then
	VERSION=$("$SCRIPT_DIR"/get-version.sh) || exit $?
else
	sha=$(git log -1 --format="%H" 2>/dev/null)
	if [ "$sha" != "" ] ; then
		# AI-generated (Claude)
		# On GitHub Actions a failing helper must fail the build. Elsewhere the
		# empty version falls back to 'unknown' below.
		VERSION=$("$SCRIPT_DIR"/get-version-for-changeset.sh "$sha") || {
			status=$?
			[ "$GITHUB_ACTIONS" = "true" ] && exit "$status"
		}
	else
		VERSION="unknown"
	fi
fi

# AI-generated (Claude)
# The release version ends up in an XML attribute, so it must never be empty,
# '<unknown>' (printed by get-version-for-changeset.sh when it finds no build
# for a changeset) or anything else that is not a plain version string. Fall
# back to the value used when there is no git. This has to stay POSIX, as the
# script may be run by dash.
case "$VERSION" in
	""|*[!0-9A-Za-z._+-]*) VERSION="unknown" ;;
esac

if [ -f "$SCRIPT_DIR"/../.gitdate ] ; then
	DATE=$(cat "$SCRIPT_DIR"/../.gitdate)
else
	DATE=$(git log -1 --format="%ct" | xargs -I{} date -d @{} +%Y-%m-%d 2>/dev/null)
fi

sed -e "s|<release version=\"\" date=\"\" />|<release version=\"$VERSION\" date=\"$DATE\" />|" metainfo/subsurface.metainfo.xml.in > metainfo/subsurface.metainfo.xml
