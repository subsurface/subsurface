#!/bin/bash

# use to assemble the details for our release notes and write them to a file
#
# Usage: create-releasenotes.sh pr_num pr_url pr_title commit_id commit_url

awk "{sub(\"PRNUM\", \"$1\"); sub(\"PRURL\", \"$2\"); sub(\"PRTITLE\", \"$3\"); sub(\"COMMITID\", \"$4\"); sub(\"COMMITURL\", \"$5\"); print}" < gh_release_notes.in > gh_release_notes

