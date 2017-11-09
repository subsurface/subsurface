#!/bin/bash

# prep things so we can build for Linux
# we have a custom built Qt some gives us just what we need, including QtWebKit
#
# this is built from the latest version as of 2017-11-09 in the 5.9 branch and
# therefore calls itself Qt-5.9.3

export QT_ROOT=$PWD/Qt/5.9.3
rm -rf Qt
mkdir -p $QT_ROOT
wget http://subsurface-divelog.org/downloads/Qt-5.9.3-trusty.tar.xz
tar -xJ -C $QT_ROOT -f Qt-5.9.3-trusty.tar.xz

# TestPreferences uses gui calls, so run a xvfb so it has something to talk to
export DISPLAY=:99.0
sh -e /etc/init.d/xvfb start

