From ubuntu:18.04

RUN mkdir -p /win
ADD settings.mk /win
RUN apt-get update  &&  apt-get upgrade -y && \
    apt-get install -y \
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
    scons ; \
    cd /win ; git clone git://github.com/mxe/mxe ; \
    cd mxe ; \
    git checkout 9f6b9c6f58510bedfa0bf9f87de9a214abe6b653 ; \
    mv ../settings.mk . ; \
    make -j 6 2>&1 | tee build.log ; \
    mkdir -p neolit ; cd neolit ; git clone -b wip/win http://code.qt.io/cgit/qt/qtconnectivity.git ; \
    cd qtconnectivity ; \
    PATH=/win/mxe/usr/bin/:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin /win/mxe/usr/i686-w64-mingw32.shared/qt5/bin/qmake qtconnectivity.pro ; \
    PATH=/win/mxe/usr/bin/:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin make -j 6 ; \
    PATH=/win/mxe/usr/bin/:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin make install ; \
    rm -rf /win/mxe/pkg ; \
    apt-get purge -y --auto-remove \
    autopoint \
    bison \
    flex \
    g++ \
    g++-multilib \
    gettext \
    gperf \
    intltool \
    libc6-dev-i386 \
    libgdk-pixbuf2.0-dev \
    libltdl-dev \
    libssl-dev \
    libtool-bin \
    libxml-parser-perl \
    openssl \
    p7zip-full \
    perl \
    python \
    ruby \
    unzip \
    xz-utils \
    lzip \
    scons
