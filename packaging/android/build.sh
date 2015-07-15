#!/bin/bash
set -e

# (trick to get the absolute path, either if we're called with a
# absolute path or a relative path)
pushd $(dirname $0)/../../
export SUBSURFACE_SOURCE=$PWD
popd
# Configure where we can find things here
export ANDROID_NDK_ROOT=$SUBSURFACE_SOURCE/../android-ndk-r10e
export ANDROID_SDK_ROOT=$SUBSURFACE_SOURCE/../android-sdk-linux
export QT5_ANDROID=$SUBSURFACE_SOURCE/../Qt/5.5
export ANDROID_NDK_HOST=linux-x86

# Which versions are we building against?
SQLITE_VERSION=3081002
LIBXML2_VERSION=2.9.2
LIBXSLT_VERSION=1.1.28
LIBZIP_VERSION=1.0.1
LIBZIP_VERSION=0.11.2
LIBGIT2_VERSION=0.23.0
LIBUSB_VERSION=1.0.19
OPENSSL_VERSION=1.0.1p

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
# autoconf seems to get lost without this
export SYSROOT=${BUILDROOT}/ndk-$ARCH/sysroot
export CFLAGS="--sysroot=${SYSROOT}"
export CPPFLAGS="--sysroot=${SYSROOT}"
export CXXFLAGS="--sysroot=${SYSROOT}"
# Junk needed for qt-android-cmake
export ANDROID_STANDALONE_TOOLCHAIN=${BUILDROOT}/ndk-$ARCH
export JAVA_HOME=/usr

if [ ! -e sqlite-autoconf-${SQLITE_VERSION}.tar.gz ] ; then
	wget http://www.sqlite.org/2015/sqlite-autoconf-${SQLITE_VERSION}.tar.gz
fi
if [ ! -e sqlite-autoconf-${SQLITE_VERSION} ] ; then
	tar -zxf sqlite-autoconf-${SQLITE_VERSION}.tar.gz
fi
if [ ! -e $PKG_CONFIG_LIBDIR/sqlite3.pc ] ; then
	mkdir -p sqlite-build-$ARCH
	pushd sqlite-build-$ARCH
	../sqlite-autoconf-${SQLITE_VERSION}/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared
	make
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

if [ ! -e openssl-${OPENSSL_VERSION}.tar.gz ] ; then
	wget -O openssl-${OPENSSL_VERSION}.tar.gz http://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
fi
if [ ! -e openssl-build-$ARCH ] ; then
	tar -zxf openssl-${OPENSSL_VERSION}.tar.gz
	mv openssl-${OPENSSL_VERSION} openssl-build-$ARCH
fi
if [ ! -e $PKG_CONFIG_LIBDIR/libssl.pc ] ; then
	pushd openssl-build-$ARCH
	if [ "$ARCH" = "arm" ] ; then
		export MACHINE="armv7l"
	else
		export MACHINE="x86"
	fi
	export SYSTEM=android
	export ARCH=$ARCH
	export CROSS_COMPILE="$ARCH-linux-androideabi-"
	export ANDROID_DEV="$ANDROID_NDK/platforms/android-14/arch-$ARCH/usr"
	export HOSTCC=gcc
	export ANDROID_NDK=$SUBSURFACE_SOURCE/../android-ndk-r10e
	export CC=gcc
	perl -pi -e 's/install: all install_docs install_sw/install: install_docs install_sw/g' Makefile.org
	bash -x ./config shared no-comp no-hw no-engine --openssldir=$PREFIX
	make depend
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
	cmake -DCMAKE_SYSTEM_NAME=Android -DSHA1_TYPE=builtin \
		-DBUILD_CLAR=OFF -DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_INSTALL_PREFIX=${PREFIX} \
		-DCURL=OFF \
		-DUSE_SSH=OFF \
		-DOPENSSL_SSL_LIBRARY=${PREFIX}/lib/libssl.a \
		-DOPENSSL_CRYPTO_LIBRARY=${PREFIX}/lib/libcrypto.a \
		-DOPENSSL_INCLUDE_DIR=${PREFIX}/include/openssl \
		-D_OPENSSL_VERSION=1.0.1p \
		../libgit2-${LIBGIT2_VERSION}/
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
	$SUBSURFACE_SOURCE/../libdivecomputer/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared --enable-examples=no
	make
	make install
	popd
fi

if [ ! -e qt-android-cmake ] ; then
	git clone git://github.com/LaurentGomila/qt-android-cmake.git
else
	pushd qt-android-cmake
	git pull -u
	popd
fi
# hack the CMake minimum version dependency - I have seen no indication
# that this is actually correct, anyway...
sed -i "s/cmake_minimum_required/#cmake_minimum_required/ ; s/cmake_policy.SET CMP0026/#cmake_policy(SET CMP0026/" qt-android-cmake/AddQtAndroidApk.cmake

# Should we build the mobile ui or the desktop ui?
if [ ! -z "$SUBSURFACE_MOBILE" ] ; then
	mkdir -p subsurface-mobile-build-$ARCH
	cd subsurface-mobile-build-$ARCH
	MOBILE_CMAKE="-DSUBSURFACE_MOBILE=ON"
	# FIXME: We should install as a different package and name to.
else
	mkdir -p subsurface-build-$ARCH
	cd subsurface-build-$ARCH
fi

# somehting in the qt-android-cmake-thingies mangles your path, so thats why we need to hard-code ant and pkg-config here.
cmake $MOBILE_CMAKE \
	-DQT_ANDROID_ANT=/usr/bin/ant \
	-DPKG_CONFIG_EXECUTABLE=/usr/bin/pkg-config \
	-DQT_ANDROID_SDK_ROOT=$ANDROID_SDK_ROOT \
	-DQT_ANDROID_NDK_ROOT=$ANDROID_NDK_ROOT \
	-DCMAKE_TOOLCHAIN_FILE=$BUILDROOT/qt-android-cmake/toolchain/android.toolchain.cmake \
	-DQT_ANDROID_CMAKE=$BUILDROOT/qt-android-cmake/AddQtAndroidApk.cmake \
	-DFORCE_LIBSSH=OFF \
	-DLIBDC_FROM_PKGCONFIG=ON \
	-DLIBGIT2_FROM_PKGCONFIG=ON \
	-DUSE_LIBGIT23_API=ON \
	-DNO_MARBLE=ON \
	-DNO_PRINTING=ON \
	-DNO_USERMANUAL=ON \
	-DCMAKE_PREFIX_PATH:UNINITIALIZED=${QT5_ANDROID}/android_${QT_ARCH}/lib/cmake \
	-DCMAKE_BUILD_TYPE=Debug \
	$SUBSURFACE_SOURCE
make
#make install INSTALL_ROOT=android_build
# bug in androiddeployqt? why is it looking for something with the builddir in it?
#ln -fs android-libsubsurface.so-deployment-settings.json android-libsubsurface-build-${ARCH}.so-deployment-settings.json
#$QT5_ANDROID_BIN/androiddeployqt --output android_build
