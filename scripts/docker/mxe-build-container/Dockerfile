# Build the image using the --build-arg option, e.g.:
# docker build -t boret/myimage:0.1 --build-arg=mxe_sha=123ABC456 .
#

# Start from Ubuntu
From ubuntu:20.04

# very often master is broken, so we pass in a known good SHA
ARG mxe_sha=master
ENV _ver=${mxe_sha}

# update and set up the packages we need for this cross build
RUN apt-get update  &&  apt-get upgrade -y && \
DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y \
    autoconf \
    automake \
    autopoint \
    bash \
    binutils \
    bison \
    bzip2 \
    ca-certificates \
    flex \
    g++ \
    g++-multilib \
    gettext \
    git \
    gperf \
    intltool \
    libc6-dev-i386 \
    libgdk-pixbuf2.0-dev \
    libltdl-dev \
    libssl-dev \
    libtool \
    libtool-bin \
    libxml-parser-perl \
    make \
    openssl \
    p7zip-full \
    patch \
    perl \
    pkg-config \
    python \
    ruby \
    sed \
    unzip \
    wget \
    xz-utils \
    lzip \
    scons

# checkout MXE at the right version
RUN mkdir -p /win
RUN cd /win ; git clone git://github.com/mxe/mxe ; \
    cd mxe ; \
    git checkout ${_ver} ;

# Patch the qtconnectivity build to explicilty enable native-win32-bluetooth and ensure another
# backend is not picked
ADD qtconnectivity-1.patch /win/mxe/src/qtconnectivity-1.patch

# Move the settings into place to build everything that we need
# separate download from build so that we can redo the build
ADD settings.mk /win/mxe/settings.mk
RUN cd /win/mxe ; \
    make -j 6 download 2>&1 | tee mxe-build.log
RUN cd /win/mxe ; \
    make -j 6 2>&1 | tee -a mxe-build.log ;

# for some reason smtk2ssrf needs a static build of mdbtools
RUN cd /win/mxe ; \
    make MXE_TARGETS=x86_64-w64-mingw32.static glib mdbtools -j 6 2>&1 | tee -a mxe-build.log ;

RUN apt-get remove -y bison flex gperf libc6-dev-i386 libgdk-pixbuf2.0-dev libxml-parser-perl python ruby xz-utils scons && rm -rf /var/lib/apt/lists/*
RUN rm -rf /win/mxe/pkg /win/mxe/log /win/mxe/docs
