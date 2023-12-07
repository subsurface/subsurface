#!/bin/bash

# Travis only pulls shallow repos. But that messes with git describe.
# Sorry Travis, fetching the whole thing and the tags as well...
git fetch --unshallow
git pull --tags
git describe --match "v[0-9]*"

# Ugly, but keeps it running during the build
docker run -v $PWD:/workspace/subsurface --name=builder -w /workspace -d fedora:27 /bin/sleep 60m

# Subsurface build dependencies
docker exec -t builder zypper refresh
docker exec -t builder dnf install -y \
	git gcc-c++ make autoconf automake libtool cmake bzip2-devel \
	libzip-devel libxml2-devel libxslt-devel libsqlite3x-devel \
	libudev-devel libusbx-devel libcurl-devel libssh2-devel\
	qt5-qtbase-devel qt5-qtdeclarative-devel qt5-qtscript-devel \
	qt5-qtwebkit-devel qt5-qtsvg-devel qt5-qttools-devel \
	qt5-qtconnectivity-devel qt5-qtlocation-devel
