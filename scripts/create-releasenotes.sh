#!/bin/bash

# use to assemble the details for our release notes and write them to a file
#
# Usage: create-releasenotes.sh merge_sha

json=$(gh pr list -s merged -S "$1" --json title,number,url)

cp gh_release_notes_top.md gh_release_notes.md
if [[ $json != "[]" ]]; then
	echo -n $json | jq -j '.[0]|{title}|join(" ")' > release_content_title.txt

	(
		echo -n 'This build was created by [merging pull request '
		echo -n $json | jq -j '.[0]|{number}|join(" ")'
		echo -n ' ('
		cat release_content_title.txt
		echo -n ')]('
		echo -n $json | jq -j '.[0]|{url}|join(" ")'
		echo ' )'
	) >> gh_release_notes.md
else
	git log --pretty=format:"%an: '%s'" -n1 > release_content_title.txt

	(
		echo "This build was created by directly pushing to the Subsurface repo."
		echo "The latest commit was [$(cat release_content_title.txt)](https://github.com/subsurface/subsurface/commit/$1 )"
	) >> gh_release_notes.md
fi
cat gh_release_notes_bottom.md >> gh_release_notes.md
