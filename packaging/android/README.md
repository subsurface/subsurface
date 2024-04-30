# Instructions for building the Android package from source

## In a Container

The easiest way to build a .apk package for Android is to use
our own docker image that has all of the build components
pre-assembled.

All it takes is this:

```.sh
export GIT_AUTHOR_NAME=<your name>
export GIT_AUTHOR_EMAIL=<email to be used with github>

cd /some/path
git clone https://github.com/subsurface/subsurface
cd subsurface
git submodule init
git submodule update
./packaging/android/docker-build.sh
```

_Caveat:_ With this build script `libdivecomputer` is pulled from git in the version specified in the submodule, so if you have changed `libdivecomputer` make sure to commit any changes and update the git submodule version before building.

This will result in Subsurface-mobile-VERSION.apk to be created in /some/path/subsurface/output/android/.

## On a Linux host

alternatively you can build locally without the help of our container.

Setup your build environment on a Ubuntu 20.04 Linux box

I think these packages should be enough:

```.sh
sudo apt-get update
sudo apt-get install -y \
    autoconf \
    automake \
    cmake \
    git \
    libtool-bin \
    make \
    wget \
    unzip \
    python \
    python3-pip \
    bzip2 \
    pkg-config \
    libx11-xcb1 \
    libgl1-mesa-glx \
    libglib2.0-0 \
    openjdk-8-jdk \
    curl \
    coreutils \
    p7zip-full

sudo mkdir /android
sudo chown `id -un` /android
cd /android
wget https://dl.google.com/android/repository/commandlinetools-linux-6858069_latest.zip
unzip commandlinetools-linux-*.zip

git clone https://github.com/subsurface/subsurface

# now get the SDK, NDK, Qt, everything that's needed
bash /android/subsurface/scripts/docker/android-build-container/android-build-setup.sh
```

Once this has completed, you should have a working build environment.

```.sh
bash -x subsurface/packaging/android/qmake-build.sh
```

should build a working .aab as well as a .apk that can be installed on
your attached device:

```.sh
./platform-tools/adb  install ./subsurface-mobile-build/android-build/build/outputs/apk/debug/android-build-debug.apk
```

Note that since you don't have the same signing key that I have,
you'll have to uninstall the 'official' Subsurface-mobile binary in
order for this to work. And likewise you have to uninstall yours
before you'll be able to install an official binary again.
