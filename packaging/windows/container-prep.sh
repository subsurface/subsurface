#!/bin/bash
# abstract the prepare commands for the windows build into a script that can be reused
# instead of a yaml file

echo "downloading sources for fresh build"
bash subsurface/scripts/get-dep-lib.sh single . libzip
bash subsurface/scripts/get-dep-lib.sh single . googlemaps
bash subsurface/scripts/get-dep-lib.sh single . libmtp
