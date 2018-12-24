From ubuntu:18.04

RUN apt-get update  && \
    apt-get upgrade -y && \
    apt-get install -y \
    autoconf \
    automake \
    git \
    libtool-bin \
    make \
    cmake \
    wget \
    unzip \
    python \
    bzip2 \
    pkg-config \
    libx11-xcb1 \
    libgl1-mesa-glx \
    libglib2.0-0 \
    openjdk-8-jdk

# create our working directory and place the local copies of the Qt
# install, NDK and SDK there, plus the three files from the Subsurface
# sources that we need to get the prep routines to run
RUN mkdir -p /android
ADD qt-opensource-linux-x64-5.*.run /android/
ADD android-ndk-r*-linux-x86_64.zip /android/
ADD sdk-tools-linux-*.zip /android/
ADD android-build-wrapper.sh variables.sh qt-installer-noninteractive.qs /android/

# run the build wrapper in prep mode
RUN cd /android && bash -x /android/android-build-wrapper.sh -prep-only

# clean up the files that we don't need to keep the container smaller
RUN cd /android && \
    rm -rf qt-opensource-linux-x64-*.run \
	   Qt/[a-zA-Z]* \
           sdk-tools-linux-*.zip \
	   android-ndk-r*-linux-x86_64.zip \
           android-sdk-linux/emulator \
           $( find android-ndk*/platforms -name arch-mips -o -name arch-x86 ) \
           android-ndk*/toolchains/x86-* android-ndk*/toolchains/llvm/prebuilt/x86-* && \
    ls -l && \
    du -sh *
RUN apt-get clean
RUN touch /android/finished-`date`
