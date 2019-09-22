FROM ubuntu:14.04
LABEL Description="Trusty with Qt 5.12 and Subsurface build deps"

# that's a lot of packages. Needed for the Qt installer, for QtWebKit
# and for SmartTrak
# the awkward start is because we need something newer than the default
# compiler for QtWebKit and we need software-properties-common in order to
# call add-apt-repositoty

RUN apt-get -y update && apt-get install -y software-properties-common
RUN add-apt-repository -y 'ppa:ubuntu-toolchain-r/test'
RUN apt-get -y update && apt-get install -y \
    gcc-6 g++-6 make git autoconf automake libtool pkg-config \
    curl libdbus-1-3 libexpat1 libfontconfig1 libfreetype6 \
    libexpat1-dev libgl1-mesa-dev libgl1-mesa-glx \
    ruby gperf bison libx11-6 libx11-xcb1 libjpeg-dev libpng-dev \
    libicu-dev libXcomposite-dev libXrender-dev libgstreamer-plugins-base1.0 \
    libxml2-dev libxslt1-dev libzip-dev libsqlite3-dev libusb-1.0-0-dev \
    libssl-dev libssh2-1-dev libcurl4-openssl-dev mesa-common-dev libqt5gui5 \
    libxcb-xinerama0 libpulse-mainloop-glib0 libhyphen-dev libicu52 \
    libglib2.0-dev mdbtools-dev

# oddly this gets us too many gcc/g++ version (even though we explicitly
# ask for gcc-6/g++6
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-6 10 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-6 10 && \
    update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 10 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 10 && \
    update-alternatives --set cc /usr/bin/gcc && \
    update-alternatives --set c++ /usr/bin/g++

# cmake in trusty is too old
ADD cmake-3.13.2-Linux-x86_64.sh .
RUN chmod a+x cmake-3.13.2-Linux-x86_64.sh && ./cmake-3.13.2-Linux-x86_64.sh --prefix=/usr --skip-license && rm -f cmake-3.13.2-Linux-x86_64.sh

# install Qt silently
ADD qtifwsilent.qs .
ADD qt-opensource-linux-x64-5.12.4.run .

RUN chmod +x qt-opensource-linux-x64-5.12.4.run && \
    QT_INSTALL_DIR=/usr/local/Qt ./qt-opensource-linux-x64-5.12.4.run --platform minimal --script qtifwsilent.qs && \
    rm -rf qt-opensource-linux-x64-5.12.4.run qtifwsilent.qs /usr/local/Qt/Tools /usr/local/Qt/Docs /usr/local/Qt/Examples /usr/local/Qt/Maintenance* \
           /usr/local/Qt/5.12.4/gcc_64/bin/qgltf /usr/local/Qt/5.12.4/gcc_64/bin/qdoc
ENV QT_ROOT /usr/local/Qt/5.12.4/gcc_64
ENV PATH="/usr/local/Qt/5.12.4/gcc_64/bin/:${PATH}"

# now build and install QtWebKit
RUN git clone -b 5.212 git://github.com/qt/qtwebkit
RUN cd qtwebkit && PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$QT_ROOT/lib/pkgconfig ./Tools/Scripts/build-webkit --qt --prefix $QT_ROOT --release
RUN cd qtwebkit/WebKitBuild/Release && make install/fast
RUN rm -rf qtwebkit

# finally, we need a new libdbus
# RUN curl -O https://dbus.freedesktop.org/releases/dbus/dbus-1.12.16.tar.gz
ADD dbus-1.12.16.tar.gz .
RUN mkdir dbus-1.12.16/build && \
    cd dbus-1.12.16/build && \
    sed -inline 's/${CMAKE_INSTALL_FULL_LOCALSTATEDIR}\/lib\/dbus\/machine-id/\/var\/lib\/dbus\/machine-id/' ../cmake/CMakeLists.txt  && \
    cmake ../cmake -DDBUS_INSTALL_SYSTEM_LIBS=1  && \
    make install && \
    ldconfig

ADD OpenSSL_1_1_1.tar.gz .
RUN cd /openssl-OpenSSL_1_1_1   && \
    ./config --prefix=/usr/local/ssl --openssldir=/usr/local/ssl shared zlib && \
    make && mkdir -p /ssllibs && cp *.so /ssllibs && cd / && rm -rf openssl-OpenSSL_1_1_1

# try to remove some of the packages we no longer need
RUN apt-get remove -y libqt5core5a libqt5dbus5 libqt5gui5 ruby openssh-client

# now build Subsurface once to populate the dependencies we don't get from
# Ubuntu but that aren't really part of Subsurface (libgit, grantlee, googlemaps)
RUN git clone git://github.com/Subsurface-divelog/subsurface
RUN bash -e -x ./subsurface/scripts/build.sh -desktop -create-appdir -build-with-webkit

# remove the source, but keep the install-root
RUN rm -rf subsurface libgit2 googlemaps grantlee