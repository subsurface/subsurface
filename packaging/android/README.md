# Over-simplistic instructions to building the Android package from source

## In a Container

The easiest way to build things is using our container. The following script will download and build the dependencies on the first run of the container, and from there on in re-use the same container to speed up build time:

```.sh
#!/bin/bash

# Change these to match your setup
SUBSURFACE_ROOT=<root directory of your local subsurface repository>
GIT_NAME="<name to use in git>"
GIT_EMAIL="<email to use in git>"

CONTAINER_NAME=android-builder-docker

# Check if our container exists
CONTAINER_ID=$(docker container ls -a -q -f name=${CONTAINER_NAME})

# Create the image if it does not exist
if [[ -z "${CONTAINER_ID}" ]]; then
    docker create -v ${SUBSURFACE_ROOT}:/android/subsurface --name=${CONTAINER_NAME} subsurface/android-build:5.15.1 sleep infinity
fi

docker start ${CONTAINER_NAME}

BUILD_PARAMETERS=""
if [[ -n "${CONTAINER_ID}" ]]; then
    BUILD_PARAMETERS="-quick"
else
    # Prepare the image for first use

    # Set the git id
    docker exec -t ${CONTAINER_NAME} git config --global user.name "${GIT_NAME}"
    docker exec -t ${CONTAINER_NAME} git config --global user.email "${GIT_EMAIL}"
fi

# Build. Do not rebuild the dependencies if this is not the first build
docker exec -e OUTPUT_DIR="/android/subsurface/android-debug" -t ${CONTAINER_NAME} /bin/bash -x ./subsurface/packaging/android/qmake-build.sh ${BUILD_PARAMETERS}

# Stop the container
docker stop ${CONTAINER_NAME}
```

_Caveat:_ With this build script `libdivecomputer` is pulled from git in the version specified in the submodule, so if you have changed `libdivecomputer` make sure to commit any changes and update the git submodule version before building.


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
bash /android/subsurface/packaging/android/android-build-setup.sh
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
you'll have to uninstalled the 'official' Subsurface-mobile binary in
order for this to work. And likewise you have to uninstall yours
before you'll be able to install an official binary again.
