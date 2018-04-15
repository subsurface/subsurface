#!/bin/bash

set -x
set -e

docker exec -t builder subsurface/scripts/build.sh -desktop 2>&1 | tee build.log
# fail the build if we didn't create the target binary
grep /workspace/install-root/bin/subsurface build.log

