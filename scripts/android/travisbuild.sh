#!/bin/bash

set -x
set -e

docker exec -e TRAVIS="$TRAVIS" -t builder subsurface/packaging/android/android-build-wrapper.sh

# Extract the built apk from the builder container
docker cp builder:/workspace/subsurface-mobile-build-arm/build/outputs/apk/ .

# TODO: Caching
# Yank Qt, android-sdk, android-ndk and other 3pp source tar balls out from the container and cache them.
#docker exec builder mkdir -p 3pp
#docker exec builder sh -c 'mv Qt android-sdk-linux android-ndk-* *.tar.gz *.tar.bz2 3pp/'
#docker cp builder:/workspace/3pp .
