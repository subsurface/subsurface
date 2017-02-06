#!/bin/bash

# Travis only pulls shallow repos. But that messes with git describe.
# Sorry Travis, fetching the whole thing and the tags as well...
git fetch --unshallow
git pull --tags
git describe

# Ugly, but keeps it running during the build
docker run -v $PWD:/workspace/subsurface --name=builder -w /workspace -d ubuntu /bin/sleep 60m
docker exec -t builder apt-get update
# subsurface android build dependencies
docker exec -t builder apt-get install -y git cmake autoconf libtool-bin openjdk-8-jdk-headless ant wget unzip python bzip2 pkg-config
# Qt installer dependencies
docker exec -t builder apt-get install -y libx11-xcb1 libgl1-mesa-glx libglib2.0-0
# Inject cached 3pp's (if none exists in 3pp dir, we inject zero ones, and all is downloaded in the container)
# TODO: caching
#docker cp 3pp builder:/workspace
docker exec -t builder mkdir -p /workspace/3pp
