#!/bin/bash
set -x
set -e

SCRIPTPATH=$(dirname $0)

export VERSION=1.1
pushd $SCRIPTPATH
docker build -t subsurface/mxe-build-container:$VERSION --build-arg=mxe_sha=1ee37f8 -f Dockerfile-stage1 .
docker build -t subsurface/mxe-build-container:$VERSION --build-arg=VERSION=$VERSION -f Dockerfile-stage2 .
popd
