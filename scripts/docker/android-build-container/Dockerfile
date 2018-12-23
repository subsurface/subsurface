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

RUN mkdir -p /android
ADD qt-opensource-linux-x64-5.*.run /android/
ADD android-ndk-r*-linux-x86_64.zip /android/
ADD android-build-wrapper.sh variables.sh qt-installer-noninteractive.qs /android/

RUN cd /android && bash -x /android/android-build-wrapper.sh -prep-only && \
    rm -f qt-opensource-linux-x64-*.run android-ndk-r*-linux-x86_64.zip && \
    rm -rf android-sdk-linux/emulator Qt/[a-zA-Z]* && \
    rm -rf $( find android-ndk*/platforms -name arch-mips -o -name arch-x86 ) && \
    rm -rf android-ndk*/toolchains/x86-* android-ndk*/toolchains/llvm/prebuilt/x86-* && \
    ls -l && \
    du -sh *
