#!/bin/bash

# prep things so we can build for Linux
# we have a custom built Qt some gives us just what we need, including QtWebKit

rm -rf Qt
mkdir -p Qt/5.9.1
wget http://subsurface-divelog.org/downloads/Qt-5.9.1.tar.xz
tar -xJ -C Qt/5.9.1 -f Qt-5.9.1.tar.xz
cd Qt/5.9.1

# this should all be handled in the packaged tar file, for now we hack it here

ln -s . gcc_64
cd ..
ln -s 5.9.1/* .
cd ..

# terrifying hack to fix the OpenSSL dependency issue
sed -i -e 's|1.0.1e|1.0.0\x00|g' Qt/lib/libQt5Network.so.5

# TestPreferences uses gui calls, so run a xvfb so it has something to talk to
export DISPLAY=:99.0
sh -e /etc/init.d/xvfb start

