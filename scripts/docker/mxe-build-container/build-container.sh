#!/bin/bash
set -x
set -e

# known good MXE sha
MXE_SHA="8966a64"
SCRIPTPATH=$(dirname $0)

# version of the docker image
VERSION=2.0

pushd $SCRIPTPATH
docker build --squash -t subsurface/mxe-build-container:$VERSION --build-arg=mxe_sha=$MXE_SHA -f Dockerfile .
popd
