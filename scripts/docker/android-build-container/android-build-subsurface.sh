#!/bin/bash

# this should run inside our android build container
# it assumes that the subsurface sources are a volume at /android/src/subsurface
#
# quite a few variables are set up in the build container env:
# ENV NDK_VERSION=27.2.12479018
# ENV SDK_LEVEL=35
# ENV SDK_VERSION=35.0.0
# ENV ANDROID_PLATFORM=24
# ENV ANDROID_BUILD_ABI=arm64-v8a
# ENV QT_VERSION=<matches the version of the container image>
# ENV BUILDROOT=/android
# ENV ANDROID_SDK_ROOT=/opt/android-sdk

# a few more need to be provided as exports by the caller to this script:
# BUILDNR
# VERSION
# VERSION_4

# we need git to work - since this is a controlled environment getting git directories
# from outside the container, the wildcard setting for safe.directory is appropriate
git config --global user.email "ci@subsurface-divelog.org"
git config --global user.name "Subsurface CI"
git config --global --add safe.directory '*'

# Qt host tools path (for moc, rcc, etc. during cross-compilation)
QT_HOST_PATH="${BUILDROOT}/Qt/${QT_VERSION}/gcc_64"

# Qt Android path
QT_ARCH="android_$(echo ${ANDROID_BUILD_ABI} | tr '-' '_')"
QT_ANDROID_PATH="${BUILDROOT}/Qt/${QT_VERSION}/${QT_ARCH}"

SUBSURFACE_SOURCE="${BUILDROOT}/src/subsurface"
ANDROID_INSTALL_PREFIX="${BUILDROOT}/src/install-root-${ANDROID_BUILD_ABI}"
mkdir -p "${ANDROID_INSTALL_PREFIX}"
ANDROID_NDK_ROOT="${ANDROID_SDK_ROOT}/ndk/${NDK_VERSION}"
export TOOLCHAIN="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/linux-x86_64"

export PATH="${TOOLCHAIN}/bin:${PATH}"

# derive compiler triple from ANDROID_BUILD_ABI
case "${ANDROID_BUILD_ABI}" in
	arm64-v8a)   TRIPLE="aarch64-linux-android" ;;
	armeabi-v7a) TRIPLE="armv7a-linux-androideabi" ;;
	x86_64)      TRIPLE="x86_64-linux-android" ;;
	x86)         TRIPLE="i686-linux-android" ;;
	*) echo "Unsupported ABI: ${ANDROID_BUILD_ABI}"; exit 1 ;;
esac

export CC="${TOOLCHAIN}/bin/${TRIPLE}${ANDROID_PLATFORM}-clang"
export CXX="${TOOLCHAIN}/bin/${TRIPLE}${ANDROID_PLATFORM}-clang++"
export AR="${TOOLCHAIN}/bin/llvm-ar"
export RANLIB="${TOOLCHAIN}/bin/llvm-ranlib"
export STRIP="${TOOLCHAIN}/bin/llvm-strip"
export ANDROID_NDK_HOME="${ANDROID_NDK_ROOT}"
SYSROOT="${TOOLCHAIN}/sysroot"
CFLAGS="--sysroot=${SYSROOT} -fPIC"
CPPFLAGS="--sysroot=${SYSROOT} -fPIC"
CXXFLAGS="--sysroot=${SYSROOT} -fPIC"
LDFLAGS="-Wl,-z,max-page-size=16384"


PREFIX="${ANDROID_INSTALL_PREFIX}"
mkdir -p "${PREFIX}"/{include,lib/pkgconfig}
export PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig"

export TARGET="${TRIPLE}"

# first set up the 3rd party components
cd "${SUBSURFACE_SOURCE}"
KIRIGAMI_BUILDDIR="${BUILDROOT}/src/kirigami-build" \
KIRIGAMI_INSTALL_PREFIX="${ANDROID_INSTALL_PREFIX}" \
bash ./scripts/mobilecomponents.sh \
	-DCMAKE_TOOLCHAIN_FILE="${QT_ANDROID_PATH}/lib/cmake/Qt6/qt.toolchain.cmake" \
	-DQT_HOST_PATH="${QT_HOST_PATH}" \
	-DANDROID_SDK_ROOT="${ANDROID_SDK_ROOT}" \
	-DANDROID_NDK_ROOT="${ANDROID_NDK_ROOT}" \
	-DANDROID_ABI="${ANDROID_BUILD_ABI}" \
	-DANDROID_PLATFORM="${ANDROID_PLATFORM}" \
	-DCMAKE_SHARED_LINKER_FLAGS="-Wl,-z,max-page-size=16384"

# build googlemaps geoservices plugin (shared library for Android)
# Qt6 Android doesn't ship qmake mkspecs, so we use cmake
echo "=== Building googlemaps plugin ==="
cd "${BUILDROOT}/src"
"${SUBSURFACE_SOURCE}/scripts/get-dep-lib.sh" single . googlemaps
cd googlemaps
git fetch --quiet
git checkout qt6-upstream --quiet 2>/dev/null || git switch qt6-upstream --quiet
# use our CMakeLists.txt if the repo doesn't have one
if [ ! -f CMakeLists.txt ]; then
	cp "${SUBSURFACE_SOURCE}/scripts/docker/android-build-container/android-googlemaps-CMakeLists.txt" CMakeLists.txt
fi
mkdir -p "${BUILDROOT}/src/googlemaps-build"
cd "${BUILDROOT}/src/googlemaps-build"
cmake -G Ninja ../googlemaps \
	-DCMAKE_TOOLCHAIN_FILE="${QT_ANDROID_PATH}/lib/cmake/Qt6/qt.toolchain.cmake" \
	-DQT_HOST_PATH="${QT_HOST_PATH}" \
	-DANDROID_SDK_ROOT="${ANDROID_SDK_ROOT}" \
	-DANDROID_NDK_ROOT="${ANDROID_NDK_ROOT}" \
	-DANDROID_ABI="${ANDROID_BUILD_ABI}" \
	-DANDROID_PLATFORM="${ANDROID_PLATFORM}" \
	-DCMAKE_INSTALL_PREFIX="${ANDROID_INSTALL_PREFIX}" \
	-DCMAKE_BUILD_TYPE=Release
cmake --build .
cmake --install .

# next, libdivecomputer
cd "${SUBSURFACE_SOURCE}"
if [ ! -f libdivecomputer/configure ]; then
	cd libdivecomputer && autoreconf -i && cd ..
fi
cd "${BUILDROOT}"
mkdir -p libdivecomputer-build && cd libdivecomputer-build
CFLAGS="${CFLAGS}" CPPFLAGS="${CPPFLAGS}" LDFLAGS="${LDFLAGS}" "${SUBSURFACE_SOURCE}"/libdivecomputer/configure --host="${TARGET}" --prefix="${PREFIX}" \
	--enable-static --disable-shared --enable-examples=no
make -j$(nproc) && make install

cd "${BUILDROOT}"

BUILD_TYPE="Release"
# write version header
mkdir -p build-android
cd build-android
cat > ssrf-version.h <<VEOF
#define CANONICAL_VERSION_STRING "${VERSION}"
#define CANONICAL_VERSION_STRING_4 "${VERSION_4}"
VEOF

# configure with Qt6 Android toolchain
cmake -G Ninja "${BUILDROOT}/src/subsurface" \
	-DCMAKE_TOOLCHAIN_FILE="${QT_ANDROID_PATH}/lib/cmake/Qt6/qt.toolchain.cmake" \
	-DQT_HOST_PATH="${QT_HOST_PATH}" \
	-DANDROID_SDK_ROOT="${ANDROID_SDK_ROOT}" \
	-DANDROID_NDK_ROOT="${ANDROID_NDK_ROOT}" \
	-DANDROID_ABI="${ANDROID_BUILD_ABI}" \
	-DANDROID_PLATFORM="${ANDROID_PLATFORM}" \
	-DCMAKE_FIND_ROOT_PATH="${ANDROID_INSTALL_PREFIX}" \
	-DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
	-DSUBSURFACE_TARGET_EXECUTABLE=MobileExecutable \
	-DLIBGIT2_FROM_PKGCONFIG=ON \
	-DFORCE_LIBSSH=OFF \
	-DNO_DOCS=ON \
	-DBUILD_TESTS=OFF \
	-DQT_ANDROID_BUILD_ALL_ABIS=OFF \
	-DBUILD_WITH_QT6=ON \
	-DANDROID_BUILDNR="${BUILDNR}" \
	-DANDROID_VERSION_NAME="${VERSION}" \
	-DCMAKE_SHARED_LINKER_FLAGS="-Wl,-z,max-page-size=16384"


cmake --build .

