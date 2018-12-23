#!/bin/bash

set -x
set -e

# by running the build wrapper again we should be able to test newer
# versions of the dependencies even without updating the docker image
# (but of course having the right things in place will save a ton of time)
docker exec -e TRAVIS="$TRAVIS" -t builder subsurface/packaging/android/android-build-wrapper.sh

ls -l ../subsurface-mobile-build-arm/build/outputs/apk/
