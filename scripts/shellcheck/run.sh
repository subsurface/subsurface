#!/usr/bin/env bash

# Run from any directory: scripts/shellcheck/run.sh [--check|--report]
# Install ShellCheck locally first (for example: sudo apt install shellcheck).
# To save the existing diagnostics: scripts/shellcheck/run.sh --report > shellcheck-report.txt
set -euo pipefail

cd "$(dirname "$0")/../.."

if (( $# > 1 )); then
	printf 'Usage: %s [--check|--report]\n' "$0" >&2
	exit 2
fi

# Include all tracked shell scripts, not only scripts in known directories.
tracked_file=$(mktemp)
trap 'rm -f "$tracked_file"' EXIT
git ls-files -z > "$tracked_file"
files=()
while IFS= read -r -d '' file; do
	case "$file" in
		*.sh|subsurface.debug)
			if [[ ! -f "$file" || -L "$file" ]]; then
				printf 'Not a regular shell script: %s\n' "$file" >&2
				exit 2
			fi
			files+=("$file")
			;;
	esac
done < "$tracked_file"

case "${1:---check}" in
	--check)
		# Every script is enforced unless explicitly exempted for existing findings.
		exempt=()
		while IFS= read -r exception || [[ -n ${exception:-} ]]; do
			exempt+=("$exception")
		done < scripts/shellcheck/exempt.txt
		enforced=()
		for file in "${files[@]}"; do
			skip=false
			for exception in "${exempt[@]}"; do
				if [[ "$file" == "$exception" ]]; then
					skip=true
					break
				fi
			done
			if [[ $skip == false ]]; then
				enforced+=("$file")
			fi
		done
		if (( ${#enforced[@]} )); then
			shellcheck --severity=warning -- "${enforced[@]}"
		fi
		;;
	--report)
		# Include exempt scripts. Findings are expected here; use GCC format.
		if shellcheck --severity=warning --format=gcc -- "${files[@]}"; then
			:
		else
			status=$?
			# ShellCheck returns 1 for findings; other failures should still fail.
			(( status == 1 )) || exit "$status"
		fi
		;;
	*)
		printf 'Usage: %s [--check|--report]\n' "$0" >&2
		exit 2
		;;
esac
