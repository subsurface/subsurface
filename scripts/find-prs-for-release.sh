#!/bin/bash
set -euo pipefail

# Find all PRs in subsurface/subsurface that first appear in a given CICD release
# by walking the commit range between two releases.
#
# Usage: find-prs-for-release.sh --tag <vX.Y.N-CICD-release> [--from-tag <vX.Y.M-CICD-release>]
#

# Helper functions
croak() {
	echo "$0: $*" >&2
	exit 1
}

croak_usage() {
	croak "Usage: $0 --tag <tag> [--from-tag <tag>]"
}

# Parse arguments
TAG=""
FROM_TAG=""

while [[ $# -gt 0 ]]; do
	case "$1" in
		--tag)
			[[ $# -lt 2 ]] && croak_usage
			[[ -n $TAG ]] && croak_usage
			TAG="$2"
			shift 2
			;;
		--from-tag)
			[[ $# -lt 2 ]] && croak_usage
			[[ -n $FROM_TAG ]] && croak_usage
			FROM_TAG="$2"
			shift 2
			;;
		*)
			croak_usage
			;;
	esac
done

[[ -z $TAG ]] && croak_usage

# Locate nightly-builds checkout
SCRIPT_SOURCE=$(dirname "${BASH_SOURCE[0]}")
SUBSURFACE_ROOT=$(cd "$SCRIPT_SOURCE/.." && pwd)

NB_DIR=""
if [[ -d "$SUBSURFACE_ROOT/../nightly-builds" ]]; then
	NB_DIR="$SUBSURFACE_ROOT/../nightly-builds"
else
	NB_DIR="$SUBSURFACE_ROOT/nightly-builds"
	[[ -d "$NB_DIR" ]] || \
		git clone --quiet https://github.com/subsurface/nightly-builds "$NB_DIR" || croak "Failed to clone nightly-builds"
fi

# Fetch latest refs
git -C "$NB_DIR" fetch --all -q || croak "Failed to fetch nightly-builds refs"

# Helper: extract build number from tag for validation
extract_buildnr() {
	[[ $1 =~ ^v[0-9]+\.[0-9]+\.([0-9]+)-CICD-release$ ]] && echo "${BASH_REMATCH[1]}"
}

# Helper: find SHA for a given build number by checking branch-for-* refs
find_sha_for_buildnr() {
	local target_buildnr="$1"

	# Enumerate all branch-for-* refs
	while IFS= read -r ref_name; do
		# Extract SHA from branch name: branch-for-<sha>
		# The ref_name could be refs/remotes/origin/branch-for-<sha> or refs/heads/branch-for-<sha>
		# We want to extract just the <sha> part
		sha=$(echo "$ref_name" | sed -E 's/^.*branch-for-([a-f0-9]+)$/\1/')

		# Skip if extraction failed (malformed ref like plain "branch-for-")
		[[ -z "$sha" || "$sha" == "$ref_name" ]] && continue

		# Check if this ref has the target build number
		if git -C "$NB_DIR" show "$ref_name:latest-subsurface-buildnumber" 2>/dev/null | grep -qx "$target_buildnr"; then
			echo "$sha"
			return 0
		fi
	done < <(git -C "$NB_DIR" for-each-ref --format='%(refname)' 'refs/remotes/origin/branch-for-*' 'refs/heads/branch-for-*' 2>/dev/null)

	return 1
}

lookup_sha_for_buildnr() {
	find_sha_for_buildnr "$1" || true
}

# Validate tag format first
if [[ ! $TAG =~ ^v[0-9]+\.[0-9]+\.[0-9]+-CICD-release$ ]]; then
	croak "Invalid tag format: $TAG. Expected format: vX.Y.N-CICD-release (e.g. v6.0.5629-CICD-release)"
fi

# Extract build number from tag
TAG_BUILDNR=$(extract_buildnr "$TAG")
[[ -z $TAG_BUILDNR ]] && croak "Cannot parse build number from tag: $TAG"

# Find SHA for this release
TO_SHA=$(lookup_sha_for_buildnr "$TAG_BUILDNR")

# If exact match not found, try the next build (in case tags are pointing to earlier commits)
if [[ -z $TO_SHA ]]; then
	FALLBACK_BUILDNR=$((TAG_BUILDNR + 1))
	TO_SHA=$(lookup_sha_for_buildnr "$FALLBACK_BUILDNR")
fi

[[ -z $TO_SHA ]] && croak "Cannot find subsurface commit for release $TAG (build $TAG_BUILDNR or $((TAG_BUILDNR + 1)))"

# Handle FROM_TAG
if [[ -n $FROM_TAG ]]; then
	if [[ ! $FROM_TAG =~ ^v[0-9]+\.[0-9]+\.[0-9]+-CICD-release$ ]]; then
		croak "Invalid from-tag format: $FROM_TAG. Expected format: vX.Y.N-CICD-release"
	fi

	FROM_BUILDNR=$(extract_buildnr "$FROM_TAG")
	[[ -z $FROM_BUILDNR ]] && croak "Cannot parse build number from from-tag: $FROM_TAG"

	FROM_SHA=$(lookup_sha_for_buildnr "$FROM_BUILDNR")

	# Fallback: try adjacent build number
	if [[ -z $FROM_SHA ]]; then
		FROM_SHA=$(lookup_sha_for_buildnr "$((FROM_BUILDNR + 1))")
	fi

	[[ -z $FROM_SHA ]] && croak "Cannot find subsurface commit for from-tag $FROM_TAG (build $FROM_BUILDNR)"
else
	# Find the previous release automatically based on the build number we actually
	# resolved (the tag's build number, or the fallback build number if needed).
	RESOLVED_BUILDNR="${FALLBACK_BUILDNR:-$TAG_BUILDNR}"
	PREV_BUILDNR=$((RESOLVED_BUILDNR - 1))
	FROM_SHA=$(lookup_sha_for_buildnr "$PREV_BUILDNR")

	if [[ -z $FROM_SHA ]]; then
		# No previous release found; use the repository root commit
		# This can happen for the very first release
		echo "No previous release found; listing commits from root to $TO_SHA" >&2
		FROM_SHA=$(git -C "$NB_DIR" rev-list --max-parents=0 HEAD 2>/dev/null | head -n1)
		[[ -z $FROM_SHA ]] && croak "Cannot determine repository root commit"
	fi
fi

# Enumerate commits in range and find PRs
mapfile -t SHAS < <(gh api "repos/subsurface/subsurface/compare/${FROM_SHA}...${TO_SHA}" --paginate --jq '.commits[].sha' 2>/dev/null)

if [[ ${#SHAS[@]} -eq 0 ]]; then
	# No commits in range, or API call failed; exit silently
	exit 0
fi

# For each commit, find associated PRs
declare -a PR_NUMS
for sha in "${SHAS[@]}"; do
	while IFS= read -r pr_num; do
		[[ -n $pr_num ]] && PR_NUMS+=("$pr_num")
	done < <(gh api "repos/subsurface/subsurface/commits/${sha}/pulls" --jq '.[].number' 2>/dev/null)
done

# Deduplicate and output (numeric sort, skip if empty)
if [[ ${#PR_NUMS[@]} -gt 0 ]]; then
	printf '%s\n' "${PR_NUMS[@]}" | sort -n -u
fi
