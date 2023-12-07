#!/bin/bash

# Travis only pulls shallow repos. But that messes with git describe.
# Sorry Travis, fetching the whole thing and the tags as well...
git fetch --unshallow
git pull --tags
git describe --match "v[0-9]*"

# Ugly, but keeps it running during the build
docker run -v $PWD:/workspace/subsurface --name=builder -w /workspace -d ubuntu:16.04 /bin/sleep 60m

# Subsurface build dependencies
docker exec -t builder apt-get update
docker exec -t builder apt-get install -y wget unzip bzip2 \
	git g++ make autoconf automake libtool cmake pkg-config \
	libxml2-dev libxslt1-dev libzip-dev libsqlite3-dev \
	libusb-1.0-0-dev libssl-dev \
	qt5-default qt5-qmake qtchooser qttools5-dev-tools libqt5svg5-dev \
	libqt5webkit5-dev libqt5qml5 libqt5quick5 qtdeclarative5-dev \
	qtscript5-dev libssh2-1-dev libcurl4-openssl-dev qttools5-dev \
	qtconnectivity5-dev qtlocation5-dev qtpositioning5-dev \
	libcrypto++-dev libssl-dev qml-module-qtpositioning qml-module-qtlocation \
	qml-module-qtquick2
