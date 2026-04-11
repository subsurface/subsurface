#!/bin/bash
# shellcheck disable=SC2164

# consistently name all builds, regardless of OS or desktop/mobile
#
# we do need to be able to create three digit (M.m.p) and four digit (M.m.p.c) version strings
# default is VERSION_EXTENSION version - an argument of '1', '3', or '4' gets you a digits only version string
#

# little silly helper functions
croak() {
	echo "$0: $*" >&2
	exit 1
}
croak_usage() {
	croak "Usage: $0 [--no-fetch] [1|3|4]"
}

NO_FETCH=false
while [[ $# -gt 0 ]] ; do
	case "$1" in
		--no-fetch) NO_FETCH=true ; shift ;;
		1|3|4) [[ -n $DIGITS ]] && croak_usage ; DIGITS="$1" ; shift ;;
		*) croak_usage ;;
	esac
done

# figure out where we are in the file system
pushd "$(dirname "$0")/../" &> /dev/null
SUBSURFACE_BASE_VERSION=$(< scripts/subsurface-base-version.txt)
export SUBSURFACE_SOURCE=$PWD

COMMITS_SINCE="0"

# add the build number to this as 'patch' component
# if we run in an environment where we are given a build number (e.g. CICD builds)
# we just grab that - otherwise we have to figure it out on the fly
if [ ! -f latest-subsurface-buildnumber ] ; then
	# figure out the most recent build number, given our own git history
	# this assumes that (a) we are in a checked out git tree and
	# (b) we have the ability to check out another git repo
	# in situations where either of these isn't true, it's the caller's
	# responsibility to ensure that the latest-subsurface-buildnumber file exists
	if [ "$NO_FETCH" != true ] ; then
		if [ ! -d "nightly-builds" ] ; then
			git clone https://github.com/subsurface/nightly-builds &> /dev/null || croak "failed to clone nightly-builds repo"
		fi
		git -C nightly-builds fetch &> /dev/null
	fi
	if ! git -C nightly-builds for-each-ref --format='%(refname)' 'refs/heads/branch-for-*' 'refs/remotes/origin/branch-for-*' 2>/dev/null | grep -q . ; then
		croak "nightly-builds is not set up correctly (not a git repo or no branch-for-* refs found)"
	fi
	pushd nightly-builds &> /dev/null
	# scan all branch-for-* refs (works for both local branches and remote-tracking refs)
	LAST_BUILD_BRANCHES=$(git for-each-ref --sort=-committerdate --format='%(refname:short)' 'refs/heads/branch-for-*' 'refs/remotes/origin/branch-for-*' | sed 's@origin/@@')
	for LAST_BUILD_BRANCH in $LAST_BUILD_BRANCHES "not-found" ; do
		LAST_BUILD_SHA=$(cut -d- -f 3 <<< "$LAST_BUILD_BRANCH")
		git -C "$SUBSURFACE_SOURCE" merge-base --is-ancestor "$LAST_BUILD_SHA" HEAD 2> /dev/null && break
	done
	[ "not-found" = "$LAST_BUILD_BRANCH" ] && croak "can't find a build number for the current working tree"
	git checkout "$LAST_BUILD_BRANCH" &> /dev/null || croak "failed to check out $LAST_BUILD_BRANCH in nightly-builds"
	BUILDNR=$(<./latest-subsurface-buildnumber)
	popd &> /dev/null
	COMMITS_SINCE=$(git log --pretty="oneline" "${LAST_BUILD_SHA}...HEAD" | wc -l | tr -d '[:space:]')
	if [[ -z $COMMITS_SINCE ]]; then
		COMMITS_SINCE="0"
	fi
else
	BUILDNR=$(<"latest-subsurface-buildnumber")
fi

VERSION_EXTENSION="-"
if [ $COMMITS_SINCE -ne 0 ]; then
	VERSION_EXTENSION+="patch.${COMMITS_SINCE}."
fi

if [ -f "latest-subsurface-buildnumber-extension" ] ; then
	SUFFIX=$(<"latest-subsurface-buildnumber-extension")
	VERSION_EXTENSION+=$(sed 's@[/_]@-@g;s/[^.a-zA-Z0-9-]//g' <<< "$SUFFIX")
else
	VERSION_EXTENSION+="local"
fi

if [[ $DIGITS == "1" ]] ; then
	VERSION="${BUILDNR}"
elif [[ $DIGITS == "3" ]] ; then
	VERSION="${SUBSURFACE_BASE_VERSION}.${BUILDNR}"
elif [[ $DIGITS == "4" ]] ; then
	VERSION="${SUBSURFACE_BASE_VERSION}.${BUILDNR}.${COMMITS_SINCE}"
else
	VERSION="${SUBSURFACE_BASE_VERSION}.${BUILDNR}${VERSION_EXTENSION}"
fi

printf '%s' "$VERSION"

popd &> /dev/null
