#!/bin/bash
# when building distributable binaries on a Mac, we cannot rely on anything from Homebrew,
# because that always requires the latest OS (how stupid is that - and they consider it a
# feature). So we painfully need to build the dependencies ourselves.
#
# this script expects a bunch of environment variables to be set:
# - ARCHS              architectures to build
# - SRC                the 'parent src dir' - this is the directory under which everything is anchored
# - SRC_DIR            usually 'subsurface'
# - MAC_CMAKE          which cmake to use
# - MAC_OPTS           macOS specific options
# - MAC_OPTS_OPENSSL   ditto for openssl
# - INSTALL_ROOT
#
# it then builds
# - libz
# - openssl
# - libcurl
# - libssh2
# - libgit2
# - libzip
# - hidapi
# - libusb
# - libftdi1
# - libmtp
# - libraw

function croak () {
    echo $@
    exit 1
}

function get_dep() {
    ./${SRC_DIR}/scripts/get-dep-lib.sh single . $1 || croak "failed to download $1"
}

cd "$SRC"

# make sure we don't pick Homebrew packages when building on GitHub
if [ "$RUNNER_OS" = "macOS" ]; then
    set -x
    export PKG_CONFIG_LIBDIR="${INSTALL_ROOT}/lib/pkgconfig"
    unset PKG_CONFIG_PATH
    export CMAKE_IGNORE_PATH="/opt/homebrew:/usr/local"
    MAC_CMAKE="-DCMAKE_PREFIX_PATH=${INSTALL_ROOT} -DCMAKE_IGNORE_PATH=/opt/homebrew;/usr/local \
        -DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew;/usr/local -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON ${MAC_CMAKE}"

    pkg-config --variable pc_path pkg-config
fi

#
# libz
#
get_dep libz
pushd libz
# no, don't install pkgconfig files in .../libs/share/pkgconf - that's just weird
sed -i .bak 's/share\/pkgconfig/pkgconfig/' CMakeLists.txt
mkdir -p build
cd build
cmake $MAC_CMAKE ..
make
make install
popd

#
# openssl
#
# openssl doesn't support fat binaries out of the box
# this tries to hack around this by first doing an install for x86_64, then a build for arm64
# and then manually creating fat libraries from that
# I worry if there are issues with using the arm or x86 include files...???
get_dep openssl
pushd openssl
for ARCH in $ARCHS; do
    mkdir -p build-$ARCH
    cd build-$ARCH
    OS_ARCH=darwin64-$ARCH-cc
    ../Configure --prefix="$INSTALL_ROOT" --openssldir="$INSTALL_ROOT" "$MAC_OPTS_OPENSSL" $OS_ARCH
    make depend
    # all the tests fail because the assume that openssl is already installed. Odd? Still things work
    make -k
    make -k install_sw install_ssldirs
    cd ..
done
if [[ $ARCHS == *" "* ]] ; then
    # now manually add the binaries together and overwrite them in the INSTALL_ROOT
    cd build-arm64
    lipo -create ./libcrypto.a ../build-x86_64/libcrypto.a -output "$INSTALL_ROOT"/lib/libcrypto.a
    lipo -create ./libssl.a ../build-x86_64/libssl.a -output "$INSTALL_ROOT"/lib/libssl.a
    LIBSSLNAME=$(readlink libssl.dylib)
    lipo -create ./$LIBSSLNAME ../build-x86_64/$LIBSSLNAME -output "$INSTALL_ROOT"/lib/$LIBSSLNAME
    LIBCRYPTONAME=$(readlink libcrypto.dylib)
    lipo -create ./$LIBCRYPTONAME ../build-x86_64/$LIBCRYPTONAME -output "$INSTALL_ROOT"/lib/$LIBCRYPTONAME
fi
popd

#
# libssh2
#

# for inexplicable reasons, on an x86 build libssh appears to be using the openssl from homebrew
# for include files and as a result fails to link... on arm builds this works just fine
# this is of course a ridiculous approach to fixing the problem, but all other attempts to tell
# cmake to really, really, pretty please use the openssl headers that we built essentially failed
rm -rf /usr/local/include/openssl /usr/local/Cellar/openssl@1.1 /usr/local/Cellar/openssl@3
ls -ld /usr/local/include/openssl /usr/local/Cellar/openssl@1.1 /usr/local/Cellar/openssl@3
ls -l /usr/local/include/openssl /usr/local/Cellar/openssl@1.1 /usr/local/Cellar/openssl@3

get_dep libssh2
pushd libssh2
mkdir -p build
cd build
cmake $MAC_CMAKE -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF \
        -DOPENSSL_ROOT_DIR="${INSTALL_ROOT}" \
        -DOPENSSL_INCLUDE_DIR="${INSTALL_ROOT}/include" \
        -DOPENSSL_CRYPTO_LIBRARY="${INSTALL_ROOT}/lib/libcrypto.dylib" \
        -DOPENSSL_SSL_LIBRARY="${INSTALL_ROOT}/lib/libssl.dylib" \
        ..
make
make install
popd
# in order for macdeployqt to do its job correctly, we need the full path in the dylib ID
pushd "$INSTALL_ROOT"/lib
NAME=$(otool -L libssh2.dylib | grep -v : | head -1 | cut -f1 -d\  | tr -d '\t')
echo "$NAME" | if grep -v / > /dev/null 2>&1 ; then
    install_name_tool -id "$INSTALL_ROOT/lib/$NAME" "$INSTALL_ROOT/lib/$NAME"
fi
popd

#
# libcurl
#
get_dep libcurl
pushd libcurl
mkdir -p build
cd build
cmake  $MAC_CMAKE -DBUILD_CURL_EXE=OFF -DCURL_DISABLE_ALTSVC=ON -DCURL_DISABLE_COOKIES=ON \
            -DCURL_DISABLE_CRYPTO_AUTH=ON -DCURL_DISABLE_DICT=ON -DCURL_DISABLE_DOH=ON \
            -DCURL_DISABLE_FILE=ON -DCURL_DISABLE_FTP=ON -DCURL_DISABLE_GETOPTIONS=ON \
            -DCURL_DISABLE_GOPHER=ON -DCURL_DISABLE_HSTS=ON -DCURL_DISABLE_IMAP=ON \
            -DCURL_DISABLE_LDAP=ON -DCURL_DISABLE_LDAPS=ON -DCURL_DISABLE_MIME=ON \
            -DCURL_DISABLE_MQTT=ON -DCURL_DISABLE_NETRC=ON -DCURL_DISABLE_NTLM=ON \
            -DCURL_DISABLE_POP3=ON -DCURL_DISABLE_PROGRESS_METER=ON -DCURL_DISABLE_PROXY=ON \
            -DCURL_DISABLE_RTSP=ON -DCURL_DISABLE_SMB=ON -DCURL_DISABLE_SMTP=ON \
            -DCURL_DISABLE_TELNET=ON -DCURL_DISABLE_TFTP=ON -DCURL_DISABLE_VERBOSE_STRINGS=ON \
            -DHTTP_ONLY=ON -DENABLE_ARES=OFF -DCURL_BROTLI=OFF -DCURL_USE_LIBPSL=OFF \
            ..
make
make install
popd

#
# libgit2
#
get_dep libgit2
pushd libgit2
mkdir -p build
cd build
LIBGIT_ARGS="-DLIBGIT2_INCLUDE_DIR=$INSTALL_ROOT/include -DLIBGIT2_LIBRARIES=$INSTALL_ROOT/lib/libgit2.dylib"
cmake $MAC_CMAKE -DBUILD_CLAR=OFF ..
make
make install
popd

# in order for macdeployqt to do its job correctly, we need the full path in the dylib ID
pushd "$INSTALL_ROOT/lib"
NAME=$(otool -L libgit2.dylib | grep -v : | head -1 | cut -f1 -d\  | tr -d '\t')
echo "$NAME" | if grep -v / > /dev/null 2>&1 ; then
    install_name_tool -id "$INSTALL_ROOT/lib/$NAME" "$INSTALL_ROOT/lib/$NAME"
fi
popd

#
# libzip
#
get_dep libzip
pushd libzip
mkdir -p build
cd build
cmake $MAC_CMAKE ..
make
make install
popd

#
# hidapi
#
get_dep hidapi
pushd hidapi
# there is no good tag, so just build master
bash ./bootstrap
mkdir -p build
cd build
cmake $MAC_CMAKE -DCMAKE_INSTALL_LIBDIR="$INSTALL_ROOT/lib" ..
make
make install
popd

#
# libusb
#
get_dep libusb
pushd libusb
bash ./bootstrap.sh
mkdir -p build
cd build
CFLAGS="$MAC_OPTS" ../configure --prefix="$INSTALL_ROOT" --disable-examples
make
make install
popd

#
# libftdi1
#
get_dep libftdi1
pushd libftdi1
mkdir -p build
cd build
cmake $MAC_CMAKE -DFTDI_EEPROM=OFF -DCMAKE_INSTALL_LIBDIR="$INSTALL_ROOT/lib" ..
make
make install
popd

#
# libmtp
#
get_dep libmtp
pushd libmtp
if [ "$RUNNER_OS" = "macOS" ]; then
    # GitHub macOS Action
    # there's something broken in the libmtp autoconf setup when using
    # the homebrew versions of these tools...
    # let's do this the hard way
    curl -O https://ftp.gnu.org/gnu/gettext/gettext-0.22.5.tar.xz
    tar xf gettext-0.22.5.tar.xz
    # Copy the m4 macros to project
    cp gettext-0.22.5/gettext-runtime/m4/iconv.m4 m4/
    cp gettext-0.22.5/gettext-runtime/gnulib-m4/lib-ld.m4 m4/
    cp gettext-0.22.5/gettext-runtime/gnulib-m4/lib-link.m4 m4/
    cp gettext-0.22.5/gettext-runtime/gnulib-m4/lib-prefix.m4 m4/
    # Clean up
    rm -rf gettext-0.22.5*
fi
sed -i.bak 's/OSFLAGS="-framework IOKit"/LIBS="$LIBS -framework IOKit"/' configure.ac
CFLAGS="$MAC_OPTS" ./autogen.sh --prefix="$INSTALL_ROOT"
make -j4
make install
popd

#
# libraw
#
get_dep libraw
pushd libraw
autoreconf -fi
EXTRA_OPTS=""
if [ "$RUNNER_OS" = "macOS" ]; then
    # GitHub macOS Action
    # libjpeg is only needed for a few very old formats - but it causes us to link against a system library
    # that forces macOS 13 as minimum version
    EXTRA_OPTS="--disable-jpeg"
fi
CFLAGS="$MAC_OPTS" CXXFLAGS="$MAC_OPTS" ./configure --prefix="$INSTALL_ROOT" --disable-examples --disable-static --enable-shared "$EXTRA_OPTS"
make -j4
make install
popd

