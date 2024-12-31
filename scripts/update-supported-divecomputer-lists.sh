#!/bin/bash

#
# Update the Lists of Supported Dive Computers Based
# on the Information in the libdivecomputer Source Code
#

CONTAINER_NAME=subsurface-android-builder

pushd . &> /dev/null
cd "$(dirname "$0")/.."

perl scripts/parse-descriptor.pl SupportedDivecomputers.html
perl scripts/parse-descriptor.pl SupportedDivecomputers.txt

popd &> /dev/null
