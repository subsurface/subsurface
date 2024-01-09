#!/bin/bash

# use to assemble the details for our release notes and write them to a file
#
# Usage: create-releasenotes.sh pr_num pr_url pr_title commit_id commit_url

sed "s/PRNUM/$1/;s/PRURL/$2/;s/PRTITLE/$3/;s/COMMITID/$4/;s/COMMITURL/$5/" < gh_release_notes.in > gh_release_notes

