#!/bin/bash

#
# find the CICD release number for a given changeset id
# Uses git to find the first nightly-build push that contains the supplied changeset id.
# Optional flag --no-search only checks the exact branch-for-<changeset> branch.

UNKNOWN='<unknown>'

# little silly helper functions
croak() {
	echo "$0: $*" >&2
	exit 1
}
croak_usage() {
	croak "Usage: $0 <changeset_id> [--no-search]"
}

if [[ $# -lt 1 || $# -gt 2 ]]; then croak_usage; fi
BUILD_SHA=$1
NO_SEARCH=0
if [[ $# -eq 2 ]]; then
	if [[ $2 == "--no-search" || $2 == "-n" ]]; then
		NO_SEARCH=1
	else
		croak_usage
	fi
fi

# figure out where we are in the file system
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SUBSURFACE_ROOT="$SCRIPT_DIR/.."
pushd "$SUBSURFACE_ROOT" &> /dev/null || exit 1
SUBSURFACE_BASE_VERSION=$(< scripts/subsurface-base-version.txt) || {
	printf '%s' "$UNKNOWN"
	popd &> /dev/null
	exit 0
}

# Clone or update the nightly-builds repo
if [ ! -d "./nightly-builds" ]; then
	git clone https://github.com/subsurface/nightly-builds &> /dev/null || {
		printf '%s' "$UNKNOWN"
		popd &> /dev/null
		exit 0
	}
fi
pushd nightly-builds &> /dev/null || exit 1
git fetch &> /dev/null

# Optional: only look at the supplied changeset branch
if [[ $NO_SEARCH -eq 1 ]]; then
	BRANCH_NAME="origin/branch-for-$BUILD_SHA"
	if git checkout "$BRANCH_NAME" &> /dev/null; then
		CICD_VERSION=$(< ./latest-subsurface-buildnumber)
		printf '%s' "$SUBSURFACE_BASE_VERSION.$CICD_VERSION"
		popd &> /dev/null
		popd &> /dev/null
		exit 0
	fi
	popd &> /dev/null
	popd &> /dev/null
	printf '%s' "$UNKNOWN"
	exit 0
fi

# Scan all nightly build branches for the first push containing the commit
BUILD_BRANCHES=$(git for-each-ref --sort=committerdate --format='%(refname:short)' refs/remotes/origin/branch-for-*)
for BUILD_BRANCH in $BUILD_BRANCHES; do
	PUSH_SHA=${BUILD_BRANCH#origin/branch-for-}
	pushd "$SUBSURFACE_ROOT" &> /dev/null || exit 1
	if git merge-base --is-ancestor "$BUILD_SHA" "$PUSH_SHA" 2>/dev/null; then
		popd &> /dev/null
		if git checkout "$BUILD_BRANCH" &> /dev/null; then
			CICD_VERSION=$(< ./latest-subsurface-buildnumber)
			printf '%s' "$SUBSURFACE_BASE_VERSION.$CICD_VERSION"
			popd &> /dev/null
			popd &> /dev/null
			exit 0
		fi
	else
		popd &> /dev/null
	fi
done

# If nothing found, return unknown
popd &> /dev/null
popd &> /dev/null
printf '%s' "$UNKNOWN"
