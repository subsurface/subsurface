#!/bin/bash

# Use this to re-create a docker container for building Android binaries

# Google makes it intentionally very hard to download the command line tools
# the URL is constantly changing and the website requires you to click through
# a license.
#
# Today this URL works:
# https://dl.google.com/android/repository/commandlinetools-linux-6858069_latest.zip
#
# If this fails, go to https://developer.android.com/studio#cmdline-tools and
# click through for yourself, and then update the URL in the Dockerfile

# create the container (this takes a while)
docker build -t android-build .
