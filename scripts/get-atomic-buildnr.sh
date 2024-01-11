#!/bin/bash

# this is intended to be used from within a GitHub action. Without the required
# token this will not work when run from the command line
# call it from the default position in the filesystem (which is inside the subsurface git tree)
#
# Usage: get-atomic-buildnr.sh SHA secrets.NIGHTLY_BUILDS [extra-name-component]
#
# the resulting release version is stored in the file ./release-version

# checkout the nightly-builds repo in parallel to the main repo
# the clone followed by the pointless push should verify that the password is stored in the config
# that way the script doesn't need the password
SCRIPT_SOURCE=$(dirname "${BASH_SOURCE[0]}")
PARENT_DIR=$(cd "$SCRIPT_SOURCE"/../.. && pwd)
url="https://subsurface:$2@github.com/subsurface/nightly-builds"
cd "$PARENT_DIR"
git clone -b main https://github.com/subsurface/nightly-builds
cd nightly-builds
git remote set-url origin "$url"
git push origin main
echo "build number prior to get-or-create was $(<latest-subsurface-buildnumber)"
cd "$PARENT_DIR"
bash subsurface/scripts/get-or-create-build-nr.sh "$1"
echo "build number after get-or-create is $(<nightly-builds/latest-subsurface-buildnumber)"
cp nightly-builds/latest-subsurface-buildnumber subsurface/
[[ -n $3 ]] && echo "$3" > subsurface/latest-subsurface-buildnumber-extension
bash subsurface/scripts/get-version > subsurface/release-version
