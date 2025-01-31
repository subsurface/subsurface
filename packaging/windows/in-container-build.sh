#!/bin/bash
# this gets executed inside the container when building a Windows
# installer as GitHub Action
#
# working directory is assumed to be the directory including all the
# source directories (subsurface, googlemaps, grantlee, etc)
# in order to be compatible with the assumed layout in the MXE script, we
# need to create the secondary build directory

set -x
set -e

mkdir -p win32
cd win32

BUILD_EXTRA_ARGS="debug"
SMTK2SSRF_EXTRA_ARGS="-b debug"
if [[ "$1" == "-release" ]]; then
    BUILD_EXTRA_ARGS=""
    SMTK2SSRF_BUILD_EXTRA_ARGS="-b release"
    shift
fi

if [[ "$*" != "" ]]; then
    BUILD_EXTRA_ARGS="${BUILD_EXTRA_ARGS} $*"
fi

# build Subsurface
export MXEBUILDTYPE=x86_64-w64-mingw32.shared
bash -ex ../subsurface/packaging/windows/mxe-based-build.sh ${BUILD_EXTRA_ARGS} installer

# the strange two step move is in order to get predictable names to use
# in the publish step of the GitHub Action
mv subsurface/subsurface.exe* ${OUTPUT_DIR}/
fullname=$(cd subsurface ; ls subsurface-*.exe)
mv subsurface/"$fullname" ${OUTPUT_DIR}/"${fullname%.exe}-installer.exe"

# build Subsurface for smtk2ssrf

bash -ex ../subsurface/packaging/windows/mxe-based-build.sh -noftdi -nolibraw subsurface

bash -ex ../subsurface/packaging/windows/smtk2ssrf-mxe-build.sh ${SMTK2SSRF_BUILD_EXTRA_ARGS} -a -i

# the strange two step move is in order to get predictable names to use
# in the publish step of the GitHub Action
mv smtk-import/smtk2ssrf.exe ${OUTPUT_DIR}/
fullname=$(cd smtk-import ; ls smtk2ssrf*.exe)
mv smtk-import/smtk2ssrf*.exe ${OUTPUT_DIR}/"${fullname%.exe}-installer.exe"
