#!/bin/bash

#
# find the changeset id for a given CICD release number
# and optionally checkout the resulting changeset
#
#
# we ignore the base version here - all that is expected is the 'patch' part of the version

# little silly helper functions
croak() {
	echo "$0: $*" >&2
	exit 1
}
croak_usage() {
	croak "Usage: $0 <patch_version_number> [-c]"
}

if [[ $# -gt 2 ]] ; then croak_usage ; fi
CICD_VERSION=$1
if [[ $# -eq 2 ]] ; then
	if [[ $2 != "-c" ]] ; then croak_usage ; fi
	DO_CHECKOUT=1
fi

# figure out where we are in the file system
pushd . &> /dev/null
cd "$(dirname "$0")/../"
pushd . &> /dev/null

if [ ! -d "./nightly-builds" ] ; then
	git clone https://github.com/subsurface/nightly-builds &> /dev/null || croak "failed to clone nightly-builds repo"
fi
cd nightly-builds
git fetch &> /dev/null

BUILD_SHA=""
BUILD_BRANCHES=$(git branch -a --sort=-committerdate --list origin/branch-for-\* | cut -d/ -f3)
for BUILD_BRANCH in $BUILD_BRANCHES ; do
	git checkout $BUILD_BRANCH &> /dev/null
	if [[ $(<./latest-subsurface-buildnumber) == $CICD_VERSION ]]; then
		BUILD_SHA=$(cut -d- -f 3 <<< "$BUILD_BRANCH")
		break
	fi
done

popd &> /dev/null

printf '%s' "$BUILD_SHA"

if [[ "$DO_CHECKOUT" == "1" && $BUILD_SHA != "" ]]; then
	git checkout $BUILD_SHA &> /dev/null
fi

popd &> /dev/null
