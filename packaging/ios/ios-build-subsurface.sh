#!/bin/bash
# AI-generated (Claude)
#
# Build Subsurface-mobile for iOS using cmake + Qt's iOS toolchain
#
# Configurable environment variables:
#   QT_VERSION    - Qt version (default: 6.10.2)
#   QT_ROOT       - Qt installation root (default: ~/Qt)
#   ARCH          - target architecture (default: arm64)
#   TARGET_SDK    - iphoneos or iphonesimulator (default: iphoneos)
#   BUILD_TYPE    - Release or Debug (default: Release)

set -xe

# auto-detect source directory
pushd "$(dirname "$0")/../../"
SUBSURFACE_SOURCE="${SUBSURFACE_SOURCE:-$PWD}"
popd
PARENT_DIR="$(cd "${SUBSURFACE_SOURCE}/.."; pwd)"

# configurable paths
QT_VERSION="${QT_VERSION:-6.10.2}"
QT_ROOT="${QT_ROOT:-${HOME}/Qt}"
QT_IOS_PATH="${QT_IOS_PATH:-${QT_ROOT}/${QT_VERSION}/ios}"
QT_HOST_PATH="${QT_HOST_PATH:-${QT_ROOT}/${QT_VERSION}/macos}"
ARCH="${ARCH:-arm64}"
TARGET_SDK="${TARGET_SDK:-iphoneos}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
IOS_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET:-17.0}"

IOS_INSTALL_PREFIX="${PARENT_DIR}/install-root/ios/${ARCH}"

# Use all available cores
NUM_CORES="$(sysctl -n hw.logicalcpu)"
export MAKEFLAGS="-j${NUM_CORES}"

echo "Building Subsurface-mobile for iOS"
echo "  Qt: ${QT_VERSION} at ${QT_IOS_PATH}"
echo "  Arch: ${ARCH}, SDK: ${TARGET_SDK}"
echo "  Build type: ${BUILD_TYPE}"

# 1. Build native dependencies (libxml2, libxslt, libzip, libgit2)
echo "=== Building native dependencies ==="
ARCH="${ARCH}" TARGET_SDK="${TARGET_SDK}" IOS_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET}" \
	bash "${SUBSURFACE_SOURCE}/packaging/ios/ios-native-libs.sh"

# 2. Build mobile components (ECM, Kirigami) with iOS cross-compilation
echo "=== Building mobile components (ECM, Kirigami) ==="
cd "${SUBSURFACE_SOURCE}"
# ECM needs qtpaths6 (host tool) in PATH and via cmake to query Qt install directories
export PATH="${QT_HOST_PATH}/bin:${PATH}"
KIRIGAMI_BUILDDIR="${PARENT_DIR}/kirigami-build" \
KIRIGAMI_INSTALL_PREFIX="${IOS_INSTALL_PREFIX}" \
bash ./scripts/mobilecomponents.sh \
	-DCMAKE_TOOLCHAIN_FILE="${QT_IOS_PATH}/lib/cmake/Qt6/qt.toolchain.cmake" \
	-DQT_HOST_PATH="${QT_HOST_PATH}" \
	-DQt6CoreTools_DIR="${QT_HOST_PATH}/lib/cmake/Qt6CoreTools" \
	-DQt6LinguistTools_DIR="${QT_HOST_PATH}/lib/cmake/Qt6LinguistTools" \
	-DBUILD_SHARED_LIBS=OFF

# 2b. Build googlemaps plugin (static, for iOS)
echo "=== Building googlemaps plugin ==="
cd "${PARENT_DIR}"
"${SUBSURFACE_SOURCE}/scripts/get-dep-lib.sh" single . googlemaps
cd googlemaps
git fetch --quiet
git checkout qt6-upstream --quiet 2>/dev/null || git switch qt6-upstream --quiet
mkdir -p ios-build
cd ios-build
"${QT_IOS_PATH}/bin/qmake" "CONFIG+=release" "CONFIG+=static" ../googlemaps.pro
make -j"${NUM_CORES}"
mkdir -p "${IOS_INSTALL_PREFIX}/plugins/geoservices"
cp libqtgeoservices_googlemaps.a "${IOS_INSTALL_PREFIX}/plugins/geoservices/"

# 3. Build libdivecomputer
echo "=== Building libdivecomputer ==="
cd "${SUBSURFACE_SOURCE}"
git submodule update --init --recursive libdivecomputer
if [ ! -f libdivecomputer/configure ]; then
	cd libdivecomputer && autoreconf -i && cd ..
fi

SDK_DIR=$(xcrun --sdk "${TARGET_SDK}" --show-sdk-path)
DC_CC=$(xcrun --sdk "${TARGET_SDK}" --find clang)
DC_CFLAGS="-arch ${ARCH} -isysroot ${SDK_DIR} -miphoneos-version-min=${IOS_DEPLOYMENT_TARGET}"

mkdir -p "${PARENT_DIR}/libdivecomputer-build-ios-${ARCH}"
cd "${PARENT_DIR}/libdivecomputer-build-ios-${ARCH}"

CURRENT_SHA=$(cd "${SUBSURFACE_SOURCE}/libdivecomputer" ; git describe --always --long)
PREVIOUS_SHA=""
if [ -f git.SHA ]; then
	PREVIOUS_SHA=$(cat git.SHA)
fi
if [ "${CURRENT_SHA}" != "${PREVIOUS_SHA}" ]; then
	echo "${CURRENT_SHA}" > git.SHA
	CC="${DC_CC}" CFLAGS="${DC_CFLAGS}" CPPFLAGS="${DC_CFLAGS}" \
		"${SUBSURFACE_SOURCE}/libdivecomputer/configure" \
		--host=arm-apple-darwin --prefix="${IOS_INSTALL_PREFIX}" \
		--enable-static --disable-shared --enable-examples=no \
		--without-libusb --without-hidapi
	make && make install
fi

# 4. Write version header
echo "=== Configuring build ==="
cd "${PARENT_DIR}"
mkdir -p build-ios
cd build-ios

SSRF_VERSION=$("${SUBSURFACE_SOURCE}/scripts/get-version.sh")
SSRF_VERSION_4=$("${SUBSURFACE_SOURCE}/scripts/get-version.sh" 4)
cat > ssrf-version.h <<VEOF
#define CANONICAL_VERSION_STRING "${SSRF_VERSION}"
#define CANONICAL_VERSION_STRING_4 "${SSRF_VERSION_4}"
VEOF

# 5. Configure with cmake using Qt's iOS toolchain
# Point pkg-config at our cross-compiled iOS libraries, not Homebrew
export PKG_CONFIG_PATH="${IOS_INSTALL_PREFIX}/lib/pkgconfig"
export PKG_CONFIG_LIBDIR="${IOS_INSTALL_PREFIX}/lib/pkgconfig"
cmake -G Xcode "${SUBSURFACE_SOURCE}" \
	-DCMAKE_TOOLCHAIN_FILE="${QT_IOS_PATH}/lib/cmake/Qt6/qt.toolchain.cmake" \
	-DQT_HOST_PATH="${QT_HOST_PATH}" \
	-DCMAKE_FIND_ROOT_PATH="${IOS_INSTALL_PREFIX}" \
	-DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
	-DSUBSURFACE_TARGET_EXECUTABLE=MobileExecutable \
	-DLIBGIT2_FROM_PKGCONFIG=ON \
	-DFORCE_LIBSSH=OFF \
	-DNO_DOCS=ON \
	-DBUILD_TESTS=OFF \
	-DBUILD_WITH_QT6=ON \
	-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO

# 6. Build
echo "=== Building ==="
cmake --build . --config "${BUILD_TYPE}" -- CODE_SIGNING_ALLOWED=NO

echo "=== Build complete ==="
echo "App bundle should be in ${PARENT_DIR}/build-ios/${BUILD_TYPE}-${TARGET_SDK}/"
