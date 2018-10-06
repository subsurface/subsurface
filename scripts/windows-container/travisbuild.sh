#!/bin/bash

# this is run to actually trigger the creation of the Windows installer
# inside the container

set -x
set -e

docker exec -t builder bash subsurface/scripts/windows-container/in-container-build.sh 2>&1 | tee build.log

# fail the build if we didn't create the target binary
grep "Built target installer" build.log

