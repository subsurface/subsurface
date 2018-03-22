#!/bin/bash

set -x
set -e

docker exec -t builder subsurface/scripts/build.sh -desktop

