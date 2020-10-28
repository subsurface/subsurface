#!/bin/bash
# this gets executed inside the container when building a Windows
# installer on Travis
#
# working directory is assumed to be the directory including all the
# source directories (subsurface, googlemaps, etc)
# in order to be compatible with the assumed layout in the MXE script, we
# need to create the secondary build directory

set -x
set -e

mkdir -p win64
cd win64

# right now the container still has an older version of MXE installed
export MXEBUILDTYPE=x86_64-w64-mingw32.shared

bash -ex ../subsurface/packaging/windows/mxe-based-build.sh installer

bash -ex ../subsurface/packaging/windows/smtk2ssrf-mxe-build.sh -a -i
