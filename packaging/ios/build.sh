#!/bin/bash
set -e

PWD=$(pwd)

mkdir -p $PWD/install-root/lib $PWD/install-root/bin $PWD/install-root/include
PKG_CONFIG_LIBDIR=$PWD/install-root/lib/pkgconfig

# Build architecture,  [armv7|armv7s|arm64|i386|x86_64]
export ARCH=i386

#arm-apple-darwin, arch64-apple-darwin, i386-apple-darwin*, x86_64-apple-darwin*
export BUILDCHAIN=i386-apple-darwin*

#iphonesimulator or iphoneos
export SDK=iphonesimulator

export SDKVERSION=$(xcrun --sdk $SDK --show-sdk-version) # current version
export SDKROOT=$(xcrun --sdk $SDK --show-sdk-path) # current version

#make subsurface set this to a saner value
export PREFIX=$PWD/install-root

# Binaries
export CC=$(xcrun --sdk $SDK --find gcc)
export CPP=$(xcrun --sdk $SDK --find gcc)" -E"
export CXX=$(xcrun --sdk $SDK --find g++)
export LD=$(xcrun --sdk $SDK --find ld)

# Flags
export CFLAGS="$CFLAGS -arch $ARCH -isysroot $SDKROOT -I$PREFIX/include -miphoneos-version-min=$SDKVERSION"
export CPPFLAGS="$CPPFLAGS -arch $ARCH -isysroot $SDKROOT -I$PREFIX/include -miphoneos-version-min=$SDKVERSION"
export CXXFLAGS="$CXXFLAGS -arch $ARCH -isysroot $SDKROOT -I$PREFIX/include"
export LDFLAGS="$LDFLAGS -arch $ARCH -isysroot $SDKROOT -L$PREFIX/lib"
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH":"$SDKROOT/usr/lib/pkgconfig":"$PREFIX/lib/pkgconfig"

# Which versions are we building against?
SQLITE_VERSION=3090200
LIBXML2_VERSION=2.9.3
LIBXSLT_VERSION=1.1.28
LIBZIP_VERSION=1.0.1
LIBZIP_VERSION=0.11.2
LIBGIT2_VERSION=0.23.4
LIBSSH2_VERSION=1.6.0
LIBUSB_VERSION=1.0.19
OPENSSL_VERSION=1.0.1p
LIBFTDI_VERSION=1.2

target=i386
hosttarget=i386
platform=iPhoneSimulator


if [ ! -e sqlite-autoconf-${SQLITE_VERSION}.tar.gz ] ; then
	curl -O http://www.sqlite.org/2015/sqlite-autoconf-${SQLITE_VERSION}.tar.gz
fi
if [ ! -e sqlite-autoconf-${SQLITE_VERSION} ] ; then
	tar -zxf sqlite-autoconf-${SQLITE_VERSION}.tar.gz
fi
if [ ! -e $PKG_CONFIG_LIBDIR/sqlite3.pc ] ; then
	mkdir -p sqlite-build-$platform
	pushd sqlite-build-$platform
	CFLAGS="${CFLAGS} -DSQLITE_ENABLE_LOCKING_STYLE=0"

	../sqlite-autoconf-${SQLITE_VERSION}/configure \
	--prefix="$PREFIX" \
	--host="$BUILDCHAIN" \
	--enable-static \
	--disable-shared \
	--disable-readline \
	--disable-dynamic-extensions

	# running make tries to build the command line executable which fails
	# so let's hack around that
	make libsqlite3.la
	touch sqlite3
	make install
	popd
fi

if [ ! -e libxml2-${LIBXML2_VERSION}.tar.gz ] ; then
	wget ftp://xmlsoft.org/libxml2/libxml2-${LIBXML2_VERSION}.tar.gz
fi
if [ ! -e libxml2-${LIBXML2_VERSION} ] ; then
	tar -zxf libxml2-${LIBXML2_VERSION}.tar.gz
fi
if [ ! -e $PKG_CONFIG_LIBDIR/libxml-2.0.pc ] ; then
	mkdir -p libxml2-build-$ARCH
	pushd libxml2-build-$ARCH
	../libxml2-${LIBXML2_VERSION}/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --without-python --without-iconv --enable-static --disable-shared
	perl -pi -e 's/runtest\$\(EXEEXT\)//' Makefile
	perl -pi -e 's/testrecurse\$\(EXEEXT\)//' Makefile
	make
	make install
	popd
fi

if [ ! -e libxslt-${LIBXSLT_VERSION}.tar.gz ] ; then
	wget ftp://xmlsoft.org/libxml2/libxslt-${LIBXSLT_VERSION}.tar.gz
fi
if [ ! -e libxslt-${LIBXSLT_VERSION} ] ; then
	tar -zxf libxslt-${LIBXSLT_VERSION}.tar.gz
	# libxslt have too old config.sub 
	cp libxml2-${LIBXML2_VERSION}/config.sub libxslt-${LIBXSLT_VERSION}
fi
if [ ! -e $PKG_CONFIG_LIBDIR/libxslt.pc ] ; then
	mkdir -p libxslt-build-$ARCH
	pushd libxslt-build-$ARCH
	../libxslt-${LIBXSLT_VERSION}/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --with-libxml-prefix=${PREFIX} --without-python --without-crypto --enable-static --disable-shared
	make
	make install
	popd
fi

if [ ! -e libzip-${LIBZIP_VERSION}.tar.gz ] ; then
	wget http://www.nih.at/libzip/libzip-${LIBZIP_VERSION}.tar.gz
fi
if [ ! -e libzip-${LIBZIP_VERSION} ] ; then
	tar -zxf libzip-${LIBZIP_VERSION}.tar.gz
fi
if [ ! -e $PKG_CONFIG_LIBDIR/libzip.pc ] ; then
	mkdir -p libzip-build-$ARCH
	pushd libzip-build-$ARCH
	../libzip-${LIBZIP_VERSION}/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared
	make
	make install
	popd
fi

# if [ ! -e openssl-${OPENSSL_VERSION}.tar.gz ] ; then
# 	wget -O openssl-${OPENSSL_VERSION}.tar.gz http://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
# fi
# if [ ! -e openssl-build-$ARCH ] ; then
# 	tar -zxf openssl-${OPENSSL_VERSION}.tar.gz
# 	mv openssl-${OPENSSL_VERSION} openssl-build-$ARCH
# fi
# if [ ! -e $PKG_CONFIG_LIBDIR/libssl.pc ] ; then
# 	pushd openssl-build-$ARCH
# 	perl -pi -e 's/install: all install_docs install_sw/install: install_docs install_sw/g' Makefile.org
# 	# Use env to make all these temporary, so they don't pollute later builds.
# 	env SYSTEM=android \
# 		CROSS_COMPILE="${BUILDCHAIN}-" \
# 		MACHINE=$OPENSSL_MACHINE \
# 		HOSTCC=gcc \
# 		CC=gcc \
# 		ANDROID_DEV=$PREFIX \
# 		bash -x ./config shared no-ssl2 no-ssl3 no-comp no-hw no-engine --openssldir=$PREFIX
# 	make depend
# 	make
# 	make install
# 	popd
# fi
#
# if [ ! -e libssh2-${LIBSSH2_VERSION}.tar.gz ] ; then
# 	wget http://www.libssh2.org/download/libssh2-${LIBSSH2_VERSION}.tar.gz
# fi
# if [ ! -e libssh2-${LIBSSH2_VERSION} ] ; then
# 	tar -zxf libssh2-${LIBSSH2_VERSION}.tar.gz
# fi
# if [ ! -e $PKG_CONFIG_LIBDIR/libssh2.pc ] ; then
# 	mkdir -p libssh2-build-$ARCH
# 	pushd libssh2-build-$ARCH
# 	../libssh2-${LIBSSH2_VERSION}/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared
# 	make
# 	make install
# 	# Patch away pkg-config dependency to zlib, its there, i promise
# 	perl -pi -e 's/^(Requires.private:.*),zlib$/$1 $2/' $PKG_CONFIG_LIBDIR/libssh2.pc
# 	popd
# fi
#
# if [ ! -e libgit2-${LIBGIT2_VERSION}.tar.gz ] ; then
# 	wget -O libgit2-${LIBGIT2_VERSION}.tar.gz https://github.com/libgit2/libgit2/archive/v${LIBGIT2_VERSION}.tar.gz
# fi
# if [ ! -e libgit2-${LIBGIT2_VERSION} ] ; then
# 	tar -zxf libgit2-${LIBGIT2_VERSION}.tar.gz
# fi
# if [ ! -e $PKG_CONFIG_LIBDIR/libgit2.pc ] ; then
# 	mkdir -p libgit2-build-$ARCH
# 	pushd libgit2-build-$ARCH
# 	cmake -DCMAKE_SYSTEM_NAME=Android -DSHA1_TYPE=builtin \
# 		-DBUILD_CLAR=OFF -DBUILD_SHARED_LIBS=OFF \
# 		-DCMAKE_INSTALL_PREFIX=${PREFIX} \
# 		-DCURL=OFF \
# 		-DUSE_SSH=ON \
# 		-DOPENSSL_SSL_LIBRARY=${PREFIX}/lib/libssl.a \
# 		-DOPENSSL_CRYPTO_LIBRARY=${PREFIX}/lib/libcrypto.a \
# 		-DOPENSSL_INCLUDE_DIR=${PREFIX}/include/openssl \
# 		-D_OPENSSL_VERSION=1.0.1p \
# 		../libgit2-${LIBGIT2_VERSION}/
# 	make
# 	make install
# 	# Patch away pkg-config dependency to zlib, its there, i promise
# 	perl -pi -e 's/^(Requires.private:.*)zlib(.*)$/$1 $2/' $PKG_CONFIG_LIBDIR/libgit2.pc
# 	popd
# fi
#
# if [ ! -e libusb-${LIBUSB_VERSION}.tar.gz ] ; then
# 	wget -O libusb-${LIBUSB_VERSION}.tar.gz https://github.com/libusb/libusb/archive/v${LIBUSB_VERSION}.tar.gz
# fi
# if [ ! -e libusb-${LIBUSB_VERSION} ] ; then
# 	tar -zxf libusb-${LIBUSB_VERSION}.tar.gz
# fi
# if ! grep -q libusb_set_android_open_callback libusb-${LIBUSB_VERSION}/libusb/libusb.h ; then
# 	# Patch in our libusb callback
# 	pushd libusb-${LIBUSB_VERSION}
# 	patch -p1 < $SUBSURFACE_SOURCE/packaging/android/patches/libusb-android.patch
# 	popd
# fi
# if [ ! -e libusb-${LIBUSB_VERSION}/configure ] ; then
# 	pushd libusb-${LIBUSB_VERSION}
# 	mkdir m4
# 	autoreconf -i
# 	popd
# fi
# if [ ! -e $PKG_CONFIG_LIBDIR/libusb-1.0.pc ] ; then
# 	mkdir -p libusb-build-$ARCH
# 	pushd libusb-build-$ARCH
# 	../libusb-${LIBUSB_VERSION}/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared --disable-udev --enable-system-log
# 	# --enable-debug-log
# 	make
# 	make install
# 	popd
# 	# Patch libusb-1.0.pc due to bug in there
# 	# Fix comming in 1.0.20
# 	sed -ie 's/Libs.private:  -c/Libs.private: /' $PKG_CONFIG_LIBDIR/libusb-1.0.pc
# fi
#
# if [ ! -e libftdi1-${LIBFTDI_VERSION}.tar.bz2 ] ; then
# 	wget -O libftdi1-${LIBFTDI_VERSION}.tar.bz2 http://www.intra2net.com/en/developer/libftdi/download/libftdi1-${LIBFTDI_VERSION}.tar.bz2
# fi
# if [ ! -e libftdi1-${LIBFTDI_VERSION} ] ; then
# 	tar -jxf libftdi1-${LIBFTDI_VERSION}.tar.bz2
# fi
# if [ ! -e $PKG_CONFIG_LIBDIR/libftdi1.pc ] && [ $PLATFORM != Darwin ] ; then
# 	mkdir -p libftdi1-build-$ARCH
# 	pushd libftdi1-build-$ARCH
# 	cmake ../libftdi1-${LIBFTDI_VERSION} -DCMAKE_C_COMPILER=${CC} -DCMAKE_INSTALL_PREFIX=${PREFIX} -DCMAKE_PREFIX_PATH=${PREFIX} -DSTATICLIBS=ON -DPYTHON_BINDINGS=OFF -DDOCUMENTATION=OFF -DFTDIPP=OFF -DBUILD_TESTS=OFF -DEXAMPLES=OFF -DFTDI_EEPROM=OFF
# 	make
# 	make install
# 	popd
# fi
# # Blast away the shared version to force static linking
# if [ -e $PREFIX/lib/libftdi1.so ] ; then
# 	rm $PREFIX/lib/libftdi1.so*
# fi
#
# if [ ! -e $PKG_CONFIG_LIBDIR/libdivecomputer.pc ] ; then
# 	mkdir -p libdivecomputer-build-$ARCH
# 	pushd libdivecomputer-build-$ARCH
# 	$SUBSURFACE_SOURCE/../libdivecomputer/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared --enable-examples=no
# 	make
# 	make install
# 	popd
# fi
#
# if [ ! -e qt-android-cmake ] ; then
# 	git clone git://github.com/LaurentGomila/qt-android-cmake.git
# else
# 	pushd qt-android-cmake
# 	git pull
# 	popd
# fi
#
# # Should we build the mobile ui or the desktop ui?
# if [ ! -z "$SUBSURFACE_MOBILE" ] ; then
# 	mkdir -p subsurface-mobile-build-$ARCH
# 	cd subsurface-mobile-build-$ARCH
# 	MOBILE_CMAKE="-DSUBSURFACE_TARGET_EXECUTABLE=MobileExecutable"
# 	# FIXME: We should install as a different package and name to.
# else
# 	mkdir -p subsurface-build-$ARCH
# 	cd subsurface-build-$ARCH
# fi
#
# # somehting in the qt-android-cmake-thingies mangles your path, so thats why we need to hard-code ant and pkg-config here.
# if [ $PLATFORM = Darwin ] ; then
# 	ANT=/usr/local/bin/ant
# 	FTDI=OFF
# else
# 	ANT=/usr/bin/ant
# 	FTDI=ON
# fi
# PKGCONF=$(which pkg-config)
# cmake $MOBILE_CMAKE \
# 	-DQT_ANDROID_ANT=${ANT} \
# 	-DPKG_CONFIG_EXECUTABLE=${PKGCONF} \
# 	-DQT_ANDROID_SDK_ROOT=$ANDROID_SDK_ROOT \
# 	-DQT_ANDROID_NDK_ROOT=$ANDROID_NDK_ROOT \
# 	-DCMAKE_TOOLCHAIN_FILE=$BUILDROOT/qt-android-cmake/toolchain/android.toolchain.cmake \
# 	-DQT_ANDROID_CMAKE=$BUILDROOT/qt-android-cmake/AddQtAndroidApk.cmake \
# 	-DFORCE_LIBSSH=ON \
# 	-DLIBDC_FROM_PKGCONFIG=ON \
# 	-DLIBGIT2_FROM_PKGCONFIG=ON \
# 	-DNO_MARBLE=ON \
# 	-DNO_PRINTING=ON \
# 	-DNO_USERMANUAL=ON \
# 	-DFBSUPPORT=OFF \
# 	-DCMAKE_PREFIX_PATH:UNINITIALIZED=${QT5_ANDROID}/android_${QT_ARCH}/lib/cmake \
# 	-DCMAKE_BUILD_TYPE=Debug \
# 	-DFTDISUPPORT=${FTDI} \
# 	$SUBSURFACE_SOURCE
# make
# #make install INSTALL_ROOT=android_build
# # bug in androiddeployqt? why is it looking for something with the builddir in it?
# #ln -fs android-libsubsurface.so-deployment-settings.json android-libsubsurface-build-${ARCH}.so-deployment-settings.json
# #$QT5_ANDROID_BIN/androiddeployqt --output android_build
