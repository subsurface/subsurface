#!/bin/bash

# Use this to re-create a docker container for building Android binaries

# copy the dependency script into this folder
cp ../../../packaging/android/android-build-wrapper.sh .
cp ../../../packaging/android/variables.sh .
cp ../../../packaging/android/qt-installer-noninteractive.qs .

# create the container (this takes a while)
sudo docker build -t android-builder --squash .
