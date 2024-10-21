# Build the image using the --build-arg option, e.g.:
# docker build -t boret/myimage:0.1 --build-arg=mxe_sha=123ABC456 .

FROM ubuntu:24.04 as base

# update and set up the packages we need for the build
RUN apt-get update  &&  \
    apt-get dist-upgrade -y && \
    apt-get install -y \
    autoconf \
    automake \
    autopoint \
    bash \
    binutils \
    bzip2 \
    ca-certificates \
    g++ \
    g++-multilib \
    gettext \
    git \
    intltool \
    libltdl-dev \
    libssl-dev \
    libtool \
    libtool-bin \
    make \
    openssl \
    p7zip-full \
    patch \
    perl \
    pkg-config \
    sed \
    unzip \
    wget \
    lzip && \
    apt-get clean


FROM base as build

# set up the packages we need additionally for this cross build
RUN apt-get install -y \
    bison \
    flex \
    gperf \
    libc6-dev-i386 \
    libgdk-pixbuf2.0-dev \
    libxml-parser-perl \
    python3 \
    python3-mako \
    python3-setuptools \
    python-is-python3 \
    ruby \
    xz-utils \
    scons

# very often master is broken, so we pass in a known good SHA
ARG mxe_sha=d6377b2f2334694dbb040294fd0d848327e63328
ENV _ver=${mxe_sha}

WORKDIR /win

# checkout MXE at the right version
RUN git clone https://github.com/mxe/mxe && \
    cd mxe && \
    git checkout ${_ver}

WORKDIR /win/mxe

# Move the settings into place to build everything that we need
ADD settings.mk .

# Patch the qtconnectivity build to explicilty enable native-win32-bluetooth and ensure another
# backend is not picked
ADD qtconnectivity-1.patch src/

# separate download from build so that we can redo the build
RUN make -j download 2>&1 | tee mxe-build.log

RUN make -j 2>&1 | tee -a mxe-build.log

# for some reason smtk2ssrf needs a static build of mdbtools
RUN make MXE_TARGETS=x86_64-w64-mingw32.static glib mdbtools -j 2>&1 | tee -a mxe-build.log

RUN rm -rf pkg log docs


FROM base as final

WORKDIR /win

COPY --from=build /win/mxe mxe
