From ubuntu:18.04

RUN apt-get update  && \
    apt-get upgrade -y && \
    apt-get install -y \
    autoconf \
    automake \
    git \
    libtool-bin \
    make \
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

# install current cmake
ADD cmake-3.13.2.tar.gz /android/
RUN cd /android/cmake-3.13.2 && \
    bash ./bootstrap && \
    make -j6 && make install

# run the build wrapper in prep mode
RUN cd /android && bash -x /android/android-build-wrapper.sh -prep-only

# uggly hack to work around some breakage in the NDK which makes our
# compiles fail
RUN sed -i '313,+13s/^using/\/\/using/' /android/android-ndk-r18b/sources/cxx-stl/llvm-libc++/include/cmath

# clean up the files that we don't need to keep the container smaller
RUN cd /android && \
    rm -rf qt-opensource-linux-x64-*.run \
	   Qt/[a-zA-Z]* \
           sdk-tools-linux-*.zip \
	   android-ndk-r*-linux-x86_64.zip \
           android-sdk-linux/emulator \
           $( find android-ndk*/platforms -name arch-mips -o -name arch-x86 ) \
           android-ndk*/toolchains/x86-* android-ndk*/toolchains/llvm/prebuilt/x86-* \
           cmake-3.13* && \
    ls -l && \
    du -sh *
RUN apt-get clean
RUN touch /android/finished-`date`
