# Build the image using the --build-arg option, e.g.:
# docker build -t boret/myimage:0.1 --build-arg=mxe_sha=123ABC456 .
#

From ubuntu:18.04
ARG mxe_sha=master
ENV _ver=${mxe_sha}
RUN mkdir -p /win
ADD settings.mk /win
RUN apt-get update  &&  apt-get upgrade -y
RUN apt-get install -y \
    autoconf \
    automake \
    autopoint \
    bash \
    binutils \
    bison \
    bzip2 \
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
RUN cd /win ; git clone git://github.com/mxe/mxe ; \
    cd mxe ; \
    git checkout ${_ver} ;
RUN mv /win/settings.mk /win/mxe
RUN cd /win/mxe ; \
    make -j 6 2>&1 | tee build.log ;
RUN cd /win/mxe ; \
    make MXE_TARGETS=i686-w64-mingw32.static glib -j 6 2>&1 | tee -a build.log ;
RUN cd /win/mxe ; \
    mkdir -p neolit ; cd neolit ; git clone -b wip/win git://github.com/qt/qtconnectivity
RUN cd /win/mxe/neolit/qtconnectivity ; \
    PATH=/win/mxe/usr/bin/:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin /win/mxe/usr/i686-w64-mingw32.shared/qt5/bin/qmake qtconnectivity.pro ; \
    PATH=/win/mxe/usr/bin/:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin make -j 6 ; \
    PATH=/win/mxe/usr/bin/:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin make install ;
RUN rm -rf /win/mxe/pkg
