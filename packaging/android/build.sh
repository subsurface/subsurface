#!/bin/bash
set -e

# Configure where we can find things here
export ANDROID_NDK_ROOT=$PWD/../../../android-ndk-r10e
export ANDROID_SDK_ROOT=$PWD/../../../android-sdk-linux
export QT5_ANDROID=$PWD/../../../Qt/5.4
export ANDROID_NDK_HOST=linux-x86

# Which versions are we building against?
SQLITE_VERSION=3080704
LIBXML2_VERSION=2.9.2
LIBXSLT_VERSION=1.1.28
LIBZIP_VERSION=0.11.2
LIBGIT2_VERSION=0.23.0
LIBUSB_VERSION=1.0.19

# arm or x86
export ARCH=${1-arm}

if [ "$ARCH" = "arm" ] ; then
	QT_ARCH="armv7"
	BUILDCHAIN=arm-linux-androideabi
elif [ "$ARCH" = "x86" ] ; then
	QT_ARCH=$ARCH
	BUILDCHAIN=i686-linux-android
fi
export QT5_ANDROID_BIN=${QT5_ANDROID}/android_${QT_ARCH}/bin

if [ ! -e ndk-$ARCH ] ; then
	$ANDROID_NDK_ROOT/build/tools/make-standalone-toolchain.sh --arch=$ARCH --install-dir=ndk-$ARCH --platform=android-14
fi
export BUILDROOT=$PWD
export PATH=${BUILDROOT}/ndk-$ARCH/bin:$PATH
export PREFIX=${BUILDROOT}/ndk-$ARCH/sysroot/usr
export PKG_CONFIG_LIBDIR=${PREFIX}/lib/pkgconfig
export CC=${BUILDROOT}/ndk-$ARCH/bin/${BUILDCHAIN}-gcc
export CXX=${BUILDROOT}/ndk-$ARCH/bin/${BUILDCHAIN}-g++

if [ ! -e sqlite-autoconf-${SQLITE_VERSION}.tar.gz ] ; then
	wget http://www.sqlite.org/2014/sqlite-autoconf-${SQLITE_VERSION}.tar.gz
fi
if [ ! -e sqlite-autoconf-${SQLITE_VERSION} ] ; then
	tar -zxf sqlite-autoconf-${SQLITE_VERSION}.tar.gz
fi
if [ ! -e $PKG_CONFIG_LIBDIR/sqlite3.pc ] ; then
	mkdir -p sqlite-build-$ARCH
	pushd sqlite-build-$ARCH
	../sqlite-autoconf-${SQLITE_VERSION}/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared
	make -j4
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
	make -j4
	make install
	popd
fi

if [ ! -e libxslt-${LIBXSLT_VERSION}.tar.gz ] ; then
	wget ftp://xmlsoft.org/libxml2/libxslt-${LIBXSLT_VERSION}.tar.gz
fi
if [ ! -e libxslt-${LIBXSLT_VERSION} ] ; then
	tar -zxf libxslt-${LIBXSLT_VERSION}.tar.gz
	# libxslt have too old config.sub for android
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

if [ ! -e libgit2-${LIBGIT2_VERSION}.tar.gz ] ; then
	wget -O libgit2-${LIBGIT2_VERSION}.tar.gz https://github.com/libgit2/libgit2/archive/v${LIBGIT2_VERSION}.tar.gz
fi
if [ ! -e libgit2-${LIBGIT2_VERSION} ] ; then
	tar -zxf libgit2-${LIBGIT2_VERSION}.tar.gz
fi
if [ ! -e $PKG_CONFIG_LIBDIR/libgit2.pc ] ; then
	mkdir -p libgit2-build-$ARCH
	pushd libgit2-build-$ARCH
	cmake -DCMAKE_SYSTEM_NAME=Android -DSHA1_TYPE=builtin -DBUILD_CLAR=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${PREFIX} ../libgit2-${LIBGIT2_VERSION}/
	make
	make install
	# Patch away pkg-config dependency to zlib, its there, i promise
	perl -pi -e 's/^(Requires.private:.*)zlib(.*)$/$1 $2/' $PKG_CONFIG_LIBDIR/libgit2.pc
	popd
fi

if [ ! -e libusb-${LIBUSB_VERSION}.tar.gz ] ; then
	wget -O libusb-${LIBUSB_VERSION}.tar.gz https://github.com/libusb/libusb/archive/v${LIBUSB_VERSION}.tar.gz
fi
if [ ! -e libusb-${LIBUSB_VERSION} ] ; then
	tar -zxf libusb-${LIBUSB_VERSION}.tar.gz
fi
if [ ! -e libusb-${LIBUSB_VERSION}/configure ] ; then
	pushd libusb-${LIBUSB_VERSION}
	mkdir m4
	autoreconf -i
	popd
fi
if [ ! -e $PKG_CONFIG_LIBDIR/libusb-1.0.pc ] ; then
	mkdir -p libusb-build-$ARCH
	pushd libusb-build-$ARCH
	../libusb-${LIBUSB_VERSION}/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared --disable-udev
	make
	make install
	popd
	# Patch libusb-1.0.pc due to bug in there
	# Fix comming in 1.0.20
	sed -ie 's/Libs.private:  -c/Libs.private: /' $PKG_CONFIG_LIBDIR/libusb-1.0.pc
fi

if [ ! -e $PKG_CONFIG_LIBDIR/libdivecomputer.pc ] ; then
	mkdir -p libdivecomputer-build-$ARCH
	pushd libdivecomputer-build-$ARCH
	../../../../libdivecomputer/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared
	make
	make install
	popd
fi

mkdir -p subsurface-build-$ARCH
cd subsurface-build-$ARCH
cmake -DCMAKE_SYSTEM_NAME=Android -DLIBDC_FROM_PKGCONFIG=ON -DLIBGIT2_FROM_PKGCONFIG=ON -DUSE_LIBGIT23_API=ON -DNO_MARBLE=ON -DNO_PRINTING=ON -DNO_USERMANUAL=ON -DCMAKE_PREFIX_PATH:UNINITIALIZED=${QT5_ANDROID}/android_${QT_ARCH}/lib/cmake ../../../
make
#make install INSTALL_ROOT=android_build
# bug in androiddeployqt? why is it looking for something with the builddir in it?
#ln -fs android-libsubsurface.so-deployment-settings.json android-libsubsurface-build-${ARCH}.so-deployment-settings.json
#$QT5_ANDROID_BIN/androiddeployqt --output android_build
