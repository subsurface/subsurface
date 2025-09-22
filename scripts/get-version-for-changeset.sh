#!/bin/bash

#
# find the CICD release number for a given changeset id
#

# little silly helper functions
croak() {
	echo "$0: $*" >&2
	exit 1
}
croak_usage() {
	croak "Usage: $0 <changeset_id> [-c]"
}

if [[ $# -ne 1 ]] ; then croak_usage ; fi
BUILD_SHA=$1

# figure out where we are in the file system
pushd . &> /dev/null
cd "$(dirname "$0")/../"
SUBSURFACE_BASE_VERSION=$(< scripts/subsurface-base-version.txt)

pushd . &> /dev/null

if [ ! -d "./nightly-builds" ] ; then
	git clone https://github.com/subsurface/nightly-builds &> /dev/null || croak "failed to clone nightly-builds repo"
fi
cd nightly-builds
git fetch &> /dev/null

BUILD_BRANCHES=$(git branch -a --sort=-committerdate --list origin/branch-for-\* | cut -d/ -f3)
for BUILD_BRANCH in $BUILD_BRANCHES ; do
	git checkout $BUILD_BRANCH &> /dev/null
	if [[ $(cut -d- -f 3 <<< "$BUILD_BRANCH") == $BUILD_SHA ]]; then
		CICD_VERSION=$(<./latest-subsurface-buildnumber)
		printf '%s' "$SUBSURFACE_BASE_VERSION.$CICD_VERSION"
		break
	fi
done

popd &> /dev/null


popd &> /dev/null
