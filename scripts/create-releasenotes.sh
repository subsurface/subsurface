#!/bin/bash

# use to assemble the details for our release notes and write them to a file
#
# Usage: create-releasenotes.sh merge_sha

json=$(gh pr list -s merged -S "$1" --json title,number,url)

cp gh_release_notes_top gh_release_notes
(
	echo -n 'This build was created by [merging PR'
	echo -n $json | jq -j '.[0]|{number}|join(" ")'
	echo -n '('
	echo -n $json | jq -j '.[0]|{title}|join(" ")'
	echo -n ')]('
	echo -n $json | jq -j '.[0]|{url}|join(" ")'
	echo ' )'
) >> gh_release_notes

cat gh_release_notes_bottom >> gh_release_notes
