#!/bin/bash

# Use this to re-create a docker container for building Android binaries

# Google makes it intentionally very hard to download the command line tools
# the URL is constantly changing and the website requires you to click through
# a license.
# Today this URL works:
if [ ! -f commandlinetools-linux-6858069_latest.zip ] ; then
	wget https://dl.google.com/android/repository/commandlinetools-linux-6858069_latest.zip
fi
# if this fails, go to https://developer.android.com/studio#cmdline-tools and click through
# for yourself...

# copy the dependency script into this folder
cp ../../../packaging/android/android-build-setup.sh .
cp ../../../packaging/android/variables.sh .

# create the container (this takes a while)
sudo docker build -t android-builder --squash .
