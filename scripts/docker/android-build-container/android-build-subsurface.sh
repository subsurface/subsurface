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
#
# mobilecomponents.sh clones Kirigami / breeze-icons / extra-cmake-modules,
# applies our local patches, and builds and installs everything into
# ANDROID_INSTALL_PREFIX. This is slow and rarely needs to re-run -- only
# when one of its inputs actually changes. We compute a hash over those
# inputs and skip the call if the hash matches what we recorded last time.
#
# Inputs that affect the result:
#   - scripts/get-dep-lib.sh (controls which versions are checked out)
#   - scripts/mobilecomponents.sh itself (build flags, patch loop)
#   - mobile-widgets/3rdparty/00*.patch (the patches that get git am'd)
#   - this script itself (the cmake flags we forward to mobilecomponents.sh)
#
# Container image / Qt / NDK changes are already covered by the .image-id
# stamp managed by local-build.sh -- on an image change INSTALL_ROOT is
# wiped, the stamp file below disappears with it, and we naturally rebuild.
cd "${SUBSURFACE_SOURCE}"
MC_STAMP="${ANDROID_INSTALL_PREFIX}/.mobilecomponents-hash"
MC_HASH=$(cat \
	scripts/get-dep-lib.sh \
	scripts/mobilecomponents.sh \
	scripts/docker/android-build-container/android-build-subsurface.sh \
	mobile-widgets/3rdparty/00*.patch \
	| sha256sum | awk '{print $1}')

if [ ! -f "${MC_STAMP}" ] || [ "$(cat "${MC_STAMP}" 2>/dev/null)" != "${MC_HASH}" ]; then
	echo "=== Building Kirigami / ECM (mobilecomponents.sh) ==="
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
	echo "${MC_HASH}" > "${MC_STAMP}"
else
	echo "=== Skipping mobilecomponents.sh (inputs unchanged) ==="
fi

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
#
# Like mobilecomponents, libdivecomputer rarely needs to re-run. Skip the
# whole configure/make/make-install dance unless the libdivecomputer source
# has actually changed -- because `make install` of autotools projects
# unconditionally rewrites headers and the static lib into ${PREFIX}, which
# bumps their mtimes and forces ninja to rebuild every subsurface translation
# unit that includes a libdivecomputer header.
#
# We hash the libdivecomputer submodule HEAD (the git SHA captured by the
# submodule pointer) plus this script itself (so changes to configure flags
# also force a rebuild). The stamp lives in INSTALL_ROOT, alongside
# .mobilecomponents-hash, so it gets wiped on container image upgrades.
LIBDC_STAMP="${ANDROID_INSTALL_PREFIX}/.libdivecomputer-hash"
LIBDC_HASH=$( ( cd "${SUBSURFACE_SOURCE}/libdivecomputer" && git rev-parse HEAD 2>/dev/null || echo "no-git" ; \
                cat "${SUBSURFACE_SOURCE}/scripts/docker/android-build-container/android-build-subsurface.sh" \
              ) | sha256sum | awk '{print $1}')

if [ ! -f "${LIBDC_STAMP}" ] || [ "$(cat "${LIBDC_STAMP}" 2>/dev/null)" != "${LIBDC_HASH}" ]; then
	echo "=== Building libdivecomputer ==="
	cd "${SUBSURFACE_SOURCE}"
	if [ ! -f libdivecomputer/configure ]; then
		cd libdivecomputer && autoreconf -i && cd ..
	fi
	cd "${BUILDROOT}"
	mkdir -p libdivecomputer-build && cd libdivecomputer-build
	CFLAGS="${CFLAGS}" CPPFLAGS="${CPPFLAGS}" LDFLAGS="${LDFLAGS}" "${SUBSURFACE_SOURCE}"/libdivecomputer/configure --host="${TARGET}" --prefix="${PREFIX}" \
		--enable-static --disable-shared --enable-examples=no
	make -j$(nproc) && make install
	echo "${LIBDC_HASH}" > "${LIBDC_STAMP}"
	# libdivecomputer's `make install` rewrote headers and the static lib
	# into ${PREFIX}; subsurface's build.ninja records dependencies on
	# those, so force a clean reconfigure of the subsurface tree to avoid
	# spurious rebuild cascades from the bumped mtimes.
	echo "=== Wiping subsurface build tree to flush stale libdivecomputer mtimes ==="
	rm -rf "${BUILDROOT}/build-android"
	mkdir -p "${BUILDROOT}/build-android"
else
	echo "=== Skipping libdivecomputer (source unchanged) ==="
fi

cd "${BUILDROOT}"

BUILD_TYPE="Release"
# write version header
mkdir -p build-android
cd build-android
cat > ssrf-version.h.new <<VEOF
#define CANONICAL_VERSION_STRING "${VERSION}"
#define CANONICAL_VERSION_STRING_4 "${VERSION_4}"
VEOF
if ! cmp -s ssrf-version.h.new ssrf-version.h 2>/dev/null; then
    mv ssrf-version.h.new ssrf-version.h
else
    rm ssrf-version.h.new
fi

# configure with Qt6 Android toolchain.
#
# Only run cmake explicitly when there is no existing build configured;
# `cmake --build .` below will trigger a reconfigure on its own (via the
# RERUN_CMAKE rule that ninja embeds in build.ninja) if and only if cmake's
# own dependency tracking decides one is needed. Re-invoking cmake here
# unconditionally would re-run the configure step every build, which writes
# out build files with fresh mtimes and forces ninja to rebuild a large
# number of targets even when nothing has actually changed.
if [ ! -f CMakeCache.txt ]; then
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
fi

cmake --build .

