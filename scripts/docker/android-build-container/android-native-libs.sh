#!/bin/bash

# build the native libraries for Android
#
# These variables must be exported by the calling shell
# NDK_VERSION        e.g. 27.2.12479018
# SDK_LEVEL          e.g. 35
# SDK_VERSION        e.g. 35.0.0
# ANDROID_PLATFORM   e.g. 24

echo "Building native libraries with NDK ${NDK_VERSION} using SDK ${SDK_VERSION}"
export BUILDROOT="/android/src"
export ANDROID_INSTALL_PREFIX="${BUILDROOT}/install-root-${ANDROID_BUILD_ABI}"
mkdir -p "${ANDROID_INSTALL_PREFIX}"
export ANDROID_HOME="/opt/android-sdk"
export ANDROID_NDK_ROOT="${ANDROID_HOME}/ndk/${NDK_VERSION}"
export TOOLCHAIN="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/linux-x86_64"

export PATH="${TOOLCHAIN}/bin:${PATH}"

# derive compiler triple and OpenSSL target from ANDROID_BUILD_ABI
case "${ANDROID_BUILD_ABI}" in
	arm64-v8a)   TRIPLE="aarch64-linux-android"; OPENSSL_TARGET="android-arm64" ;;
	armeabi-v7a) TRIPLE="armv7a-linux-androideabi"; OPENSSL_TARGET="android-arm" ;;
	x86_64)      TRIPLE="x86_64-linux-android"; OPENSSL_TARGET="android-x86_64" ;;
	x86)         TRIPLE="i686-linux-android"; OPENSSL_TARGET="android-x86" ;;
	*) echo "Unsupported ABI: ${ANDROID_BUILD_ABI}"; exit 1 ;;
esac

export CC="${TOOLCHAIN}/bin/${TRIPLE}${ANDROID_PLATFORM}-clang"
export CXX="${TOOLCHAIN}/bin/${TRIPLE}${ANDROID_PLATFORM}-clang++"
export AR="${TOOLCHAIN}/bin/llvm-ar"
export RANLIB="${TOOLCHAIN}/bin/llvm-ranlib"
export STRIP="${TOOLCHAIN}/bin/llvm-strip"
export ANDROID_NDK_HOME="${ANDROID_NDK_ROOT}"
SYSROOT="${TOOLCHAIN}/sysroot"

PREFIX="${ANDROID_INSTALL_PREFIX}"
mkdir -p "${PREFIX}"/{include,lib/pkgconfig}
export PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig"

export TARGET="${TRIPLE}"

# show the environment in the log
env

cd "${BUILDROOT}"

# chicken and egg - we want to be able to include these libraries in
# the docker image because they take so brutally long to build, but
# we need the Subsurface sources in order to be able to build them
#
# In order to keep things in sync, we need to make sure that we build
# a new container if any of this changes

# download all source dependencies
"${BUILDROOT}/get-dep-lib.sh" singleAndroid . openssl
"${BUILDROOT}/get-dep-lib.sh" singleAndroid . sqlite
"${BUILDROOT}/get-dep-lib.sh" singleAndroid . libxml2
"${BUILDROOT}/get-dep-lib.sh" singleAndroid . libxslt
"${BUILDROOT}/get-dep-lib.sh" singleAndroid . libzip
"${BUILDROOT}/get-dep-lib.sh" singleAndroid . libgit2

# --- OpenSSL 3.x (build before setting sysroot CFLAGS) ---
# Qt 6.5+ requires OpenSSL 3.x; it loads libssl_3.so / libcrypto_3.so at runtime
mkdir -p openssl-build
cp -r openssl/* openssl-build/
cd openssl-build
./Configure "${OPENSSL_TARGET}" no-ssl3 no-comp no-engine no-ui-console \
  --prefix="${PREFIX}" -fPIC -Wl,-z,max-page-size=16384
make -j"$(nproc)" build_libs
cp -RL include/openssl "${PREFIX}"/include/openssl
# Copy resolved shared libraries for linking (-L follows symlinks)
cp -L libssl.so "${PREFIX}"/lib/libssl.so
cp -L libcrypto.so "${PREFIX}"/lib/libcrypto.so
# Qt 6.5+ on Android dlopen's libssl_3.so / libcrypto_3.so (underscore naming)
cp -L libssl.so "${PREFIX}"/lib/libssl_3.so
cp -L libcrypto.so "${PREFIX}"/lib/libcrypto_3.so
cp *.pc "${PKG_CONFIG_PATH}"/
cd "${BUILDROOT}"

# sysroot flags for autotools-based builds
CFLAGS="--sysroot=${SYSROOT} -fPIC"
CPPFLAGS="--sysroot=${SYSROOT} -fPIC"
CXXFLAGS="--sysroot=${SYSROOT} -fPIC"

# --- SQLite ---
mkdir -p sqlite-build && cd sqlite-build
CFLAGS="${CFLAGS}" CPPFLAGS="${CPPFLAGS}" \
  ../sqlite/configure --host="${TARGET}" --prefix="${PREFIX}" --enable-static --disable-shared
make -j$(nproc) && make install
cd "${BUILDROOT}"

# --- libxml2 ---
if [ ! -f libxml2/configure ]; then
  cd libxml2 && autoreconf --install && cd "${BUILDROOT}"
fi
mkdir -p libxml2-build && cd libxml2-build
CFLAGS="${CFLAGS}" CPPFLAGS="${CPPFLAGS}" \
  ../libxml2/configure --host="${TARGET}" --prefix="${PREFIX}" \
  --without-python --without-iconv --enable-static --disable-shared
perl -pi -e 's/runtest\$\(EXEEXT\)//' Makefile
perl -pi -e 's/testrecurse\$\(EXEEXT\)//' Makefile
make -j$(nproc) && make install
cd "${BUILDROOT}"

# --- libxslt ---
if [ ! -f libxslt/configure ]; then
  cd libxslt && autoreconf --install && cd "${BUILDROOT}"
fi
mkdir -p libxslt-build && cd libxslt-build
CFLAGS="${CFLAGS}" CPPFLAGS="${CPPFLAGS}" \
  ../libxslt/configure --host="${TARGET}" --prefix="${PREFIX}" \
  --with-libxml-prefix="${PREFIX}" --without-python --without-crypto \
  --enable-static --disable-shared
make -j$(nproc) && make install
cd "${BUILDROOT}"

# --- libzip ---
cd libzip
git reset --hard
sed -i 's/SIZEOF_SIZE_T/__SIZEOF_SIZE_T__/g' lib/compat.h
sed -i 's/ADD_SUBDIRECTORY(man)//' CMakeLists.txt
sed -i 's/^FIND_PACKAGE(ZLIB/#&/' CMakeLists.txt
cd "${BUILDROOT}"
mkdir -p libzip-build && cd libzip-build
cmake -G Ninja \
  -DCMAKE_C_COMPILER="${CC}" \
  -DCMAKE_LINKER="${CC}" \
  -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
  -DCMAKE_INSTALL_LIBDIR="lib" \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_DISABLE_FIND_PACKAGE_BZip2=TRUE \
  -DZLIB_VERSION_STRING=1.2.7 \
  -DZLIB_LIBRARY=z \
  ../libzip/
ninja && ninja install
cd "${BUILDROOT}"

# --- libgit2 ---
mkdir -p libgit2-build && cd libgit2-build
cmake -G Ninja \
  -DCMAKE_C_COMPILER="${CC}" \
  -DCMAKE_LINKER="${CC}" \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DBUILD_CLAR=OFF -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
  -DCURL=OFF \
  -DUSE_SSH=OFF \
  -DOPENSSL_SSL_LIBRARY="${PREFIX}"/lib/libssl.so \
  -DOPENSSL_CRYPTO_LIBRARY="${PREFIX}"/lib/libcrypto.so \
  -DOPENSSL_INCLUDE_DIR="${PREFIX}"/include \
  -DCMAKE_DISABLE_FIND_PACKAGE_HTTP_Parser=TRUE \
  ../libgit2/
find ../libgit2 -name 'CMakeLists.txt' -exec sed -i 's|C_STANDARD 90|C_STANDARD 99|' {} \;
ninja && ninja install
perl -pi -e 's/^(Requires.private:.*)zlib(.*)$/$1 $2/' "${PKG_CONFIG_PATH}"/libgit2.pc
cd "${BUILDROOT}"
