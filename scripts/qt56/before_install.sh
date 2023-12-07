#!/bin/bash

# Travis only pulls shallow repos. But that messes with git describe.
# Sorry Travis, fetching the whole thing and the tags as well...
git fetch --unshallow
git pull --tags
git describe --match "v[0-9]*"

# Ugly, but keeps it running during the build
docker run -v $PWD:/workspace/subsurface --name=builder -w /workspace -d opensuse:42.3 /bin/sleep 60m

# Subsurface build dependencies
docker exec -t builder zypper refresh
docker exec -t builder zypper --non-interactive install \
	git gcc-c++ make autoconf automake libtool cmake libzip-devel \
	libssh2-devel libxml2-devel libxslt-devel sqlite3-devel libusb-1_0-devel \
	libqt5-linguist-devel libqt5-qttools-devel libQt5WebKitWidgets-devel \
	libqt5-qtbase-devel libQt5WebKit5-devel libqt5-qtsvg-devel \
	libqt5-qtscript-devel libqt5-qtdeclarative-devel \
	libqt5-qtconnectivity-devel libqt5-qtlocation-devel libcurl-devel which \
	openssl-devel curl
