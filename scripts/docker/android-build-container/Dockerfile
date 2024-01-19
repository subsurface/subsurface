FROM ubuntu:22.04 as base

RUN apt-get update  && \
    apt-get upgrade -y && \
    apt-get install -y \
    unzip \
    git \
    cmake \
    autoconf \
    libtool-bin \
    openjdk-8-jdk \
    wget && \
    apt-get clean


FROM base as build

WORKDIR /android

# Scrape the manually curated Qt install from the previous build image
COPY --from=subsurface/android-build-container:5.15.1 /android/5.15.1 5.15.1

# install, NDK and SDK there, plus the three files from the Subsurface
# sources that we need to get the prep routines to run
RUN wget https://dl.google.com/android/repository/commandlinetools-linux-6858069_latest.zip && \
    unzip commandlinetools-linux-*.zip
ADD android-build-setup.sh variables.sh .

# run the build setup
RUN bash -x android-build-setup.sh

# clean up the files that we don't need to keep the container smaller
RUN rm -rf \
	   5*/android/lib/*x86* \
	   5*/android/doc \
	   5*/android/include/QtHelp \
	   5*/android/include/QtFbSupport \
	   5*/android/include/QtFontDatabaseSupport \
	   5*/android/include/QtNfc \
	   5*/android/include/QtPrintSupport \
	   5*/android/include/QtTest \
	   5*/android/include/QtXml \
	   5*/android/plugins/geoservices/libqtgeoservices_mapboxgl.so \
	   5*/android/lib/cmake/Qt5Test/Qt5TestConfig.cmake \
	   commandlinetools-linux-*.zip \
	   $( find platforms -name arch-mips -o -name arch-x86 ) \
	   toolchains/x86-* android-ndk*/toolchains/llvm/prebuilt/x86-* \
	   platforms/android-[12][2345678] \
	   platforms/android-21/arch-x86_64 \
	   ndk/*/sources/cxx-stl/llvm-libc++/libs/x* \
	   ndk/*/sources/cxx-stl/llvm-libc++/libs/*/*static* \
	   ndk/*/sources/cxx-stl/llvm-libc++/test \
	   ndk/*/sources/cxx-stl/llvm-libc++/utils \
	   ndk/*/sources/cxx-stl/llvmlibc++abi \
	   ndk/*/sources/cxx-stl/system \
	   ndk/*/sources/third_party \
	   ndk/*/sysroot/usr/lib \
	   build-tools/*/renderscript \
	   platform-tools/systrace \
	   tools/lib \
	   tools/proguard \
	   tools/support \
	   emulator \
	   platform-tools-2 \
	   variables.sh \
	   android-build-setup.sh \
	   /usr/lib/gcc


FROM base as final

RUN apt-get install -y \
    autoconf \
    automake \
    cmake \
    git \
    make \
    wget \
    zip \
    unzip \
    python3 \
    python3-pip \
    bzip2 \
    pkg-config \
    libx11-xcb1 \
    libgl1-mesa-glx \
    libglib2.0-0 \
    openjdk-8-jdk \
    curl \
    coreutils \
    p7zip-full && \
    apt-get clean

WORKDIR /android

COPY --from=build /android/ .

RUN git config --global --add safe.directory /android/subsurface && \
    git config --global --add safe.directory /android/subsurface/libdivecomputer && \
    git config --global --add safe.directory /android/subsurface/nightly-builds
