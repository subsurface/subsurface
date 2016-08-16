#!/bin/bash
#
# show what you are doing and stop when things break
set -x
set -e

# set up easy to use variables with the important paths
TOP=$(pwd)
SUBSURFACE_SOURCE=${TOP}/../../../subsurface
IOS_QT=${TOP}/Qt
QT_VERSION=$(cd ${IOS_QT}; ls -d 5.?)

# Which versions are we building against?
SQLITE_VERSION=3090200
LIBXML2_VERSION=2.9.2
LIBXSLT_VERSION=1.1.28
LIBZIP_VERSION=0.11.2
LIBGIT2_VERSION=0.23.4
LIBSSH2_VERSION=1.6.0
OPENSSL_VERSION=1.0.1p

# not on iOS so far, but kept here for completeness
LIBUSB_VERSION=1.0.19
LIBFTDI_VERSION=1.2

# set up the Subsurface versions by hand
GITVERSION=$(git describe --tags --abbrev=12)
CANONICALVERSION=$(git describe --tags --abbrev=12 | sed -e 's/-g.*$// ; s/^v//' | sed -e 's/-/./')
MOBILEVERSION=$(grep MOBILE ../../cmake/Modules/version.cmake | cut -d\" -f 2)
echo "#define GIT_VERSION_STRING \"$GITVERSION\"" > subsurface-mobile/ssrf-version.h
echo "#define CANONICAL_VERSION_STRING \"$CANONICALVERSION\"" >> subsurface-mobile/ssrf-version.h
echo "#define MOBILE_VERSION_STRING \"$MOBILEVERSION\"" >> subsurface-mobile/ssrf-version.h

# create Info.plist with the correct versions
cat Info.plist.in | sed "s/@MOBILE_VERSION@/$MOBILEVERSION/;s/@CANONICAL_VERSION@/$CANONICALVERSION/" > Info.plist

# Build Subsurface-mobile by default
SUBSURFACE_MOBILE=1

pushd ${SUBSURFACE_SOURCE}
bash scripts/mobilecomponents.sh
popd


# now build all the dependencies for the three relevant architectures (x86_64 is for the simulator)

for ARCH in armv7 arm64 x86_64; do

echo next building for $ARCH

	INSTALL_ROOT=$TOP/install-root-$ARCH
	mkdir -p $INSTALL_ROOT/lib $INSTALL_ROOT/bin $INSTALL_ROOT/include
	PKG_CONFIG_LIBDIR=$INSTALL_ROOT/lib/pkgconfig

	declare -x PKG_CONFIG_PATH=$PKG_CONFIG_LIBDIR
	declare -x PREFIX=$INSTALL_ROOT

	if [ "$ARCH" = "x86_64" ] ; then
		declare -x SDK_NAME="iphonesimulator"
		declare -x TOOLCHAIN_FILE="${TOP}/iPhoneSimulatorCMakeToolchain"
		declare -x IOS_PLATFORM=SIMULATOR64
		declare -x BUILDCHAIN=x86_64-apple-darwin
	else
		declare -x SDK_NAME="iphoneos"
		declare -x TOOLCHAIN_FILE="${TOP}/iPhoneDeviceCMakeToolchain"
		declare -x IOS_PLATFORM=OS
		declare -x BUILDCHAIN=arm-apple-darwin
	fi
	declare -x ARCH_NAME=$ARCH
	declare -x SDK=$SDK_NAME
	declare -x SDK_DIR=`xcrun --sdk $SDK_NAME --show-sdk-path`
	declare -x PLATFORM_DIR=`xcrun --sdk $SDK_NAME --show-sdk-platform-path`

	declare -x CC=`xcrun -sdk $SDK_NAME -find clang`
	declare -x CXX=`xcrun -sdk $SDK_NAME -find clang++`
	declare -x LD=`xcrun -sdk $SDK_NAME -find ld`
	declare -x CFLAGS="-arch $ARCH_NAME -isysroot $SDK_DIR -miphoneos-version-min=6.0 -I$SDK_DIR/usr/include"
	declare -x CXXFLAGS="$CFLAGS"
	declare -x LDFLAGS="$CFLAGS  -lpthread -lc++ -L$SDK_DIR/usr/lib"


	# openssl build stuff.
	export DEVELOPER=$(xcode-select --print-path)\
	export IPHONEOS_SDK_VERSION=$(xcrun --sdk iphoneos --show-sdk-version)
	export IPHONEOS_DEPLOYMENT_VERSION="6.0"
	export IPHONEOS_PLATFORM=$(xcrun --sdk iphoneos --show-sdk-platform-path)
	export IPHONEOS_SDK=$(xcrun --sdk iphoneos --show-sdk-path)
	export IPHONESIMULATOR_PLATFORM=$(xcrun --sdk iphonesimulator --show-sdk-platform-path)
	export IPHONESIMULATOR_SDK=$(xcrun --sdk iphonesimulator --show-sdk-path)
	export OSX_SDK_VERSION=$(xcrun --sdk macosx --show-sdk-version)
	export OSX_DEPLOYMENT_VERSION="10.8"
	export OSX_PLATFORM=$(xcrun --sdk macosx --show-sdk-platform-path)
	export OSX_SDK=$(xcrun --sdk macosx --show-sdk-path)

	target=$ARCH
	hosttarget=$ARCH

	if [ ! -e sqlite-autoconf-${SQLITE_VERSION}.tar.gz ] ; then
		curl -O http://www.sqlite.org/2015/sqlite-autoconf-${SQLITE_VERSION}.tar.gz
	fi
	if [ ! -e sqlite-autoconf-${SQLITE_VERSION} ] ; then
		tar -zxf sqlite-autoconf-${SQLITE_VERSION}.tar.gz
	fi
	if [ ! -e $PKG_CONFIG_LIBDIR/sqlite3.pc ] ; then
		mkdir -p sqlite-build-$ARCH_NAME
		pushd sqlite-build-$ARCH_NAME
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
		if [ "$ARCH_NAME" == "x86_64" ]; then
			../libxml2-${LIBXML2_VERSION}/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --without-python --without-iconv --enable-static --disable-shared
		else
			../libxml2-${LIBXML2_VERSION}/configure --host=arm-apple-darwin --prefix=${PREFIX} --without-python --without-iconv --enable-static --disable-shared
		fi
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
		mkdir -p libxslt-build-$ARCH_NAME
		pushd libxslt-build-$ARCH_NAME
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
		mkdir -p libzip-build-$ARCH_NAME
		pushd libzip-build-$ARCH_NAME
		../libzip-${LIBZIP_VERSION}/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared
		make
		make install
		popd
	fi

	configure_openssl() {
	    OS=$1
	    ARCH=$2
	    PLATFORM=$3
	    SDK_VERSION=$4
	    DEPLOYMENT_VERSION=$5

	    export CROSS_TOP="${PLATFORM}/Developer"
	    export CROSS_SDK="${OS}${SDK_VERSION}.sdk"
	    if [ "$ARCH_NAME" == "x86_64" ]; then
	      ./Configure darwin64-${ARCH}-cc --openssldir="${PREFIX}" --prefix="${PREFIX}"
	      sed -ie "s!^CFLAG=!CFLAG=-isysroot ${CROSS_TOP}/SDKs/${CROSS_SDK} -arch $ARCH -mios-simulator-version-min=${DEPLOYMENT_VERSION} !" "Makefile"
	    else
	      ./Configure iphoneos-cross -no-asm --openssldir="${PREFIX}"
	      sed -ie "s!^CFLAG=!CFLAG=-isysroot ${CROSS_TOP}/SDKs/${CROSS_SDK} -arch $ARCH -miphoneos-version-min=${DEPLOYMENT_VERSION} !" "Makefile"
	      perl -i -pe 's|static volatile sig_atomic_t intr_signal|static volatile int intr_signal|' crypto/ui/ui_openssl.c
	    fi
	}

	build_openssl()
	{
	   ARCH=$1
	   SDK=$2
	   TYPE=$3
	   export BUILD_TOOLS="${DEVELOPER}"
	   mkdir -p "lib-${TYPE}"

	   rm -rf openssl-${OPENSSL_VERSION}
	   tar xfz openssl-${OPENSSL_VERSION}.tar.gz
	   pushd .
	   cd "openssl-${OPENSSL_VERSION}"
	   #fix header for Swift
	   sed -ie "s/BIGNUM \*I,/BIGNUM \*i,/g" crypto/rsa/rsa.h
	   if [ "$TYPE" == "ios" ]; then
	     if [ "$ARCH" == "x86_64" ]; then
		 configure_openssl "iPhoneSimulator" $ARCH ${IPHONESIMULATOR_PLATFORM} ${IPHONEOS_SDK_VERSION} ${IPHONEOS_DEPLOYMENT_VERSION}
	     else
		 configure_openssl "iPhoneOS" $ARCH ${IPHONEOS_PLATFORM} ${IPHONEOS_SDK_VERSION} ${IPHONEOS_DEPLOYMENT_VERSION}
	     fi
	   fi
	   make
	   make install_sw
	   popd
	}

	if [ ! -e openssl-${OPENSSL_VERSION}.tar.gz ] ; then
		wget -O openssl-${OPENSSL_VERSION}.tar.gz http://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
	fi
	if [ ! -e openssl-build-$ARCH ] ; then
		tar -zxf openssl-${OPENSSL_VERSION}.tar.gz
		mv openssl-${OPENSSL_VERSION} openssl-build-$ARCH
	fi
	if [ ! -e $PKG_CONFIG_LIBDIR/libssl.pc ] ; then
		build_openssl "$ARCH" "${IPHONESIMULATOR_SDK}" "ios"
	fi

	if [ ! -e libssh2-${LIBSSH2_VERSION}.tar.gz ] ; then
		wget http://www.libssh2.org/download/libssh2-${LIBSSH2_VERSION}.tar.gz
	fi
	if [ ! -e libssh2-${LIBSSH2_VERSION} ] ; then
		tar -zxf libssh2-${LIBSSH2_VERSION}.tar.gz
	fi
	if [ ! -e $PKG_CONFIG_LIBDIR/libssh2.pc ] ; then
		mkdir -p libssh2-build-$ARCH
		pushd libssh2-build-$ARCH
		../libssh2-${LIBSSH2_VERSION}/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared
		make
		make install
		# Patch away pkg-config dependency to zlib, its there, i promise
		perl -pi -e 's/^(Requires.private:.*),zlib$/$1 $2/' $PKG_CONFIG_LIBDIR/libssh2.pc
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
		cmake ../libgit2-${LIBGIT2_VERSION} \
		    -G "Unix Makefiles" \
		    -DBUILD_SHARED_LIBS="OFF" \
		    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
			-DSHA1_TYPE=builtin \
			-DBUILD_CLAR=OFF \
			-DCMAKE_INSTALL_PREFIX=${PREFIX} \
			-DCMAKE_PREFIX_PATH=${PREFIX} \
			-DCURL=OFF \
			-DUSE_SSH=ON \
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

# let's skip libusb and libftdi for the moment...
# not sure if / when we'll be able to use USB devices on iOS
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

# build kirigami
#	if [ ! "$ARCH" = "x86_64" ] ; then
#		mkdir -p kirigami-build-$ARCH
#		pushd kirigami-build-$ARCH
#		${IOS_QT}/${QT_VERSION}/ios/bin/qmake ${SUBSURFACE_SOURCE}/mobile-widgets/qml/kirigami/kirigami.pro -r -spec macx-ios-clang CONFIG+=iphoneos CONFIG+=release QMAKE_IOS_DEVICE_ARCHS=$ARCH
#		make
#		cp org/kde/libkirigamiplugin.a ${PREFIX}/lib
#		popd
#	fi

# build libdivecomputer
	if [ ! -d libdivecomputer ] ; then
		git clone -b Subsurface-branch git://subsurface-divelog.org/libdc libdivecomputer
	fi
	cd libdivecomputer
	git pull --rebase
	if ! git checkout Subsurface-branch ; then
		echo "can't check out the Subsurface-branch branch of libdivecomputer -- giving up"
		exit 1
	fi
	if [ ! -f configure ] ; then
		autoreconf --install
	fi
	cd ..

	if [ ! -e $PKG_CONFIG_LIBDIR/libdivecomputer.pc ] ; then
		mkdir -p libdivecomputer-build-$ARCH
		pushd libdivecomputer-build-$ARCH
		../libdivecomputer/configure --host=${BUILDCHAIN} --prefix=${PREFIX} --enable-static --disable-shared --enable-examples=no --disable-libusb --disable-hidapi
		make
		make install
		popd
	fi
done

# now combine the arm libraries into fat libraries
rm -rf install-root
cp -a install-root-arm64 install-root
pushd install-root/lib
for LIB in $(find . -type f -name \*.a); do
	lipo ../../install-root-armv7/lib/$LIB ../../install-root-arm64/lib/$LIB -create -output $LIB
done
popd

pushd ${SUBSURFACE_SOURCE}/translations
SRCS=$(ls *.ts | grep -v source)
popd
pushd Subsurface-mobile
mkdir -p translations
for src in $SRCS; do
	${IOS_QT}/${QT_VERSION}/ios/bin/lrelease ${SUBSURFACE_SOURCE}/translations/$src -qm translations/${src/.ts/.qm}
done
popd
