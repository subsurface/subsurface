From ubuntu:18.04

RUN apt-get update  && \
    apt-get upgrade -y && \
    apt-get install -y \
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

# create our working directory and place the local copies of the Qt
# install, NDK and SDK there, plus the three files from the Subsurface
# sources that we need to get the prep routines to run
RUN mkdir -p /android
ADD commandlinetools-linux-*.zip /android/
RUN cd /android && unzip commandlinetools-linux-*.zip
ADD android-build-setup.sh variables.sh /android/

# run the build setup
RUN ls -l /android
RUN cd /android && bash -x /android/android-build-setup.sh

# clean up the files that we don't need to keep the container smaller
RUN cd /android && \
    rm -rf \
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
	   android-build-setup.sh
	   /usr/lib/gcc && \
    ls -l && \
    du -sh *
RUN apt-get clean
#RUN cd /android/android-ndk-r18b/toolchains && ln -s x86_64-4.9 x86-64-4.9
RUN touch /android/finished-"`date`"
