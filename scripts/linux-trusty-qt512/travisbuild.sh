#!/bin/bash

set -x
set -e

# this is run to actually trigger the creation of the AppImage
# inside the container

docker exec -t trusty-qt512 bash subsurface/scripts/linux-trusty-qt512/in-container-build.sh 2>&1 | tee build.log

# fail the build if we didn't create Subsurface-mobile
grep "Built target subsurface-mobile" build.log

# fail the build if we didn't create the AppImage
grep "Please consider submitting your AppImage to AppImageHub" build.log

