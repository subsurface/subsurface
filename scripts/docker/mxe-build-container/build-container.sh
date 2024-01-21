#!/bin/bash
set -x
set -e

# known good MXE sha
MXE_SHA="c0bfefc57a00fdf6cb5278263e21a478e47b0bf5"
SCRIPTPATH=$(dirname $0)

# version of the docker image
VERSION=3.1.0

pushd $SCRIPTPATH

# we use the 'experimental' --squash argument to significantly reduce the size of the massively huge
# Docker container this produces
docker build -t subsurface/mxe-build:$VERSION --build-arg=mxe_sha=$MXE_SHA -f Dockerfile .
docker images
popd
