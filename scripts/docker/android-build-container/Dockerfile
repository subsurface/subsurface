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
    openjdk-8-jdk \
    curl \
    p7zip-full

# create our working directory and place the local copies of the Qt
# install, NDK and SDK there, plus the three files from the Subsurface
# sources that we need to get the prep routines to run
RUN mkdir -p /android
ADD android-ndk-r*-linux-x86_64.zip /android/
ADD sdk-tools-linux-*.zip /android/
ADD install-qt.sh android-build-wrapper.sh variables.sh /android/

# install current cmake
ADD cmake-3.13.2.tar.gz /android/
RUN cd /android/cmake-3.13.2 && \
    bash ./bootstrap && \
    make -j6 && make install

# run the build wrapper in prep mode
RUN cd /android && bash -x /android/android-build-wrapper.sh -prep-only

# uggly hack to work around some breakage in the NDK which makes our
# compiles fail
#RUN sed -i '313,+13s/^using/\/\/using/' /android/android-ndk-r21/sources/cxx-stl/llvm-libc++/include/cmath

# clean up the files that we don't need to keep the container smaller
RUN cd /android && \
    apt-get remove -y gcc g++-7 libstdc++-7-dev && \
    rm -rf \
	   Qt/5*/android_arm*/lib/*x86* \
	   Qt/5*/android_arm*/doc \
	   Qt/5*/android_arm*/include/QtHelp \
	   Qt/5*/android_arm*/include/QtFbSupport \
	   Qt/5*/android_arm*/include/QtFontDatabaseSupport \
	   Qt/5*/android_arm*/include/QtNfc \
	   Qt/5*/android_arm*/include/QtPrintSupport \
	   Qt/5*/android_arm*/include/QtTest \
	   Qt/5*/android_arm*/include/QtXml \
	   Qt/5*/android_arm*/plugins/geoservices/libqtgeoservices_mapboxgl.so \
	   sdk-tools-linux-*.zip \
	   android-ndk-r*-linux-x86_64.zip \
	   $( find android-ndk*/platforms -name arch-mips -o -name arch-x86 ) \
	   android-ndk-r[0-9a-z]*/toolchains/x86-* android-ndk*/toolchains/llvm/prebuilt/x86-* \
	   android-ndk-r[0-9a-z]*/platforms/android-[12][2345678] \
	   android-ndk-r[0-9a-z]*/platforms/android-21/arch-x86_64 \
	   android-ndk-r[0-9a-z]*/prebuilt/linux-x86_64 \
	   android-ndk-r[0-9a-z]*/simpleperf \
	   android-ndk-r[0-9a-z]*/sources/cxx-stl/llvm-libc++/libs/x* \
	   android-ndk-r[0-9a-z]*/sources/cxx-stl/llvm-libc++/libs/*/*static* \
	   android-ndk-r[0-9a-z]*/sources/cxx-stl/llvm-libc++/test \
	   android-ndk-r[0-9a-z]*/sources/cxx-stl/llvm-libc++/utils \
	   android-ndk-r[0-9a-z]*/sources/cxx-stl/llvmlibc++abi \
	   android-ndk-r[0-9a-z]*/sources/cxx-stl/system \
	   android-ndk-r[0-9a-z]*/sources/third_party \
	   android-ndk-r[0-9a-z]*/sysroot/usr/lib \
	   android-sdk-linux/build-tools/28.0.3/renderscript \
	   android-sdk-linux/platform-tools/systrace \
	   android-sdk-linux/tools/lib \
	   android-sdk-linux/tools/proguard \
	   android-sdk-linux/tools/support \
           android-sdk-linux/emulator \
	   /usr/lib/gcc \
           cmake-3.13* && \
    ls -l && \
    du -sh *
RUN apt-get clean
RUN cd /android/android-ndk-r18b/toolchains && ln -s x86_64-4.9 x86-64-4.9
RUN touch /android/finished-"`date`"
