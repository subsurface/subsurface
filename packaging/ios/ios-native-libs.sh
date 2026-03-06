#!/bin/bash
# AI-generated (Claude)
#
# Build native C/C++ dependencies for iOS
# Runs natively on macOS (no Docker, unlike the Android build)
#
# Configurable environment variables:
#   SUBSURFACE_SOURCE  - path to subsurface source tree (default: auto-detected)
#   ARCH               - target architecture (default: arm64)
#   IOS_DEPLOYMENT_TARGET - minimum iOS version (default: 17.0)
#
# The script expects the dependency sources to already be present in
# the parent directory of SUBSURFACE_SOURCE (fetched by get-dep-lib.sh).

set -xe

ARCH="${ARCH:-arm64}"
IOS_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET:-17.0}"

# auto-detect source directory
pushd "$(dirname "$0")/../../"
SUBSURFACE_SOURCE="${SUBSURFACE_SOURCE:-$PWD}"
popd
PARENT_DIR="$(cd "${SUBSURFACE_SOURCE}/.."; pwd)"

# select SDK based on architecture / target
if [ "${TARGET_SDK}" = "iphonesimulator" ]; then
	SDK_NAME="iphonesimulator"
else
	SDK_NAME="iphoneos"
fi

SDK_DIR=$(xcrun --sdk "${SDK_NAME}" --show-sdk-path)
CC=$(xcrun --sdk "${SDK_NAME}" --find clang)
export CC
CXX=$(xcrun --sdk "${SDK_NAME}" --find clang++)
export CXX

export CFLAGS="-arch ${ARCH} -isysroot ${SDK_DIR} -miphoneos-version-min=${IOS_DEPLOYMENT_TARGET} -fPIC"
export CXXFLAGS="${CFLAGS}"
export LDFLAGS="${CFLAGS}"

IOS_INSTALL_PREFIX="${PARENT_DIR}/install-root/ios/${ARCH}"
mkdir -p "${IOS_INSTALL_PREFIX}"/{include,lib/pkgconfig}
export PKG_CONFIG_PATH="${IOS_INSTALL_PREFIX}/lib/pkgconfig"
PREFIX="${IOS_INSTALL_PREFIX}"

# Use all available cores
NUM_CORES="$(sysctl -n hw.logicalcpu)"
export MAKEFLAGS="-j${NUM_CORES}"

echo "Building iOS native libraries for ${ARCH} (SDK: ${SDK_NAME}, deployment target: ${IOS_DEPLOYMENT_TARGET})"

# download source dependencies
"${SUBSURFACE_SOURCE}/scripts/get-dep-lib.sh" ios "${PARENT_DIR}"

cd "${PARENT_DIR}"

# --- libxml2 (cmake) ---
if [ ! -f "${PKG_CONFIG_PATH}/libxml-2.0.pc" ]; then
	mkdir -p libxml2-build-ios-"${ARCH}" && cd libxml2-build-ios-"${ARCH}"
	cmake "${PARENT_DIR}/libxml2" \
		-DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_SYSTEM_NAME=iOS \
		-DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET}" \
		-DCMAKE_OSX_SYSROOT="${SDK_DIR}" \
		-DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DCMAKE_PREFIX_PATH="${PREFIX}" \
		-DLIBXML2_WITH_PYTHON=OFF \
		-DLIBXML2_WITH_ICONV=OFF \
		-DLIBXML2_WITH_LZMA=OFF \
		-DLIBXML2_WITH_ZLIB=OFF \
		-DLIBXML2_WITH_TESTS=OFF \
		-DLIBXML2_WITH_PROGRAMS=OFF
	make && make install
	cd "${PARENT_DIR}"
fi

# --- libxslt (cmake) ---
if [ ! -f "${PKG_CONFIG_PATH}/libxslt.pc" ]; then
	mkdir -p libxslt-build-ios-"${ARCH}" && cd libxslt-build-ios-"${ARCH}"
	cmake "${PARENT_DIR}/libxslt" \
		-DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_SYSTEM_NAME=iOS \
		-DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET}" \
		-DCMAKE_OSX_SYSROOT="${SDK_DIR}" \
		-DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DCMAKE_PREFIX_PATH="${PREFIX}" \
		-DLibXml2_DIR="${PREFIX}/lib/cmake/libxml2" \
		-DCMAKE_C_FLAGS="-Wno-nullability-completeness" \
		-DLIBXSLT_WITH_PROFILER=OFF \
		-DLIBXSLT_WITH_PROGRAMS=OFF \
		-DLIBXSLT_WITH_PYTHON=OFF \
		-DLIBXSLT_WITH_TESTS=OFF
	make && make install
	cd "${PARENT_DIR}"
fi

# --- libzip (cmake) ---
# The old libzip version doesn't have BUILD_TOOLS options, so we strip
# the tool/test/example subdirectories from CMakeLists.txt before building
if [ ! -f "${PKG_CONFIG_PATH}/libzip.pc" ]; then
	pushd "${PARENT_DIR}/libzip"
	sed -i.bak 's/ADD_SUBDIRECTORY(src)//;s/ADD_SUBDIRECTORY(examples)//;s/ADD_SUBDIRECTORY(man)//;s/ADD_SUBDIRECTORY(regress)//' CMakeLists.txt
	popd
	mkdir -p libzip-build-ios-"${ARCH}" && cd libzip-build-ios-"${ARCH}"
	cmake "${PARENT_DIR}/libzip" \
		-DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_SYSTEM_NAME=iOS \
		-DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET}" \
		-DCMAKE_OSX_SYSROOT="${SDK_DIR}" \
		-DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DCMAKE_PREFIX_PATH="${PREFIX}" \
		-DCMAKE_DISABLE_FIND_PACKAGE_BZip2=TRUE \
		-DENABLE_OPENSSL=FALSE \
		-DENABLE_GNUTLS=FALSE \
		-DCMAKE_POLICY_VERSION_MINIMUM=3.5
	make && make install
	cd "${PARENT_DIR}"
	# restore the original CMakeLists.txt
	pushd "${PARENT_DIR}/libzip"
	mv CMakeLists.txt.bak CMakeLists.txt
	popd
fi

# --- libgit2 (cmake, uses SecureTransport for HTTPS) ---
if [ ! -f "${PKG_CONFIG_PATH}/libgit2.pc" ]; then
	mkdir -p libgit2-build-ios-"${ARCH}" && cd libgit2-build-ios-"${ARCH}"
	cmake "${PARENT_DIR}/libgit2" \
		-DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_SYSTEM_NAME=iOS \
		-DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET}" \
		-DCMAKE_OSX_SYSROOT="${SDK_DIR}" \
		-DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DCMAKE_PREFIX_PATH="${PREFIX}" \
		-DBUILD_TESTS=OFF \
		-DBUILD_CLI=OFF \
		-DBUILD_CLAR=OFF \
		-DCURL=OFF \
		-DUSE_SSH=OFF \
		-DUSE_HTTPS=SecureTransport \
		-DSECURITY_FOUND=TRUE \
		-DSECURITY_HAS_SSLCREATECONTEXT=TRUE \
		-DSECURITY_INCLUDE_DIR="${SDK_DIR}/usr/include" \
		-DSECURITY_LIBRARIES="${SDK_DIR}/System/Library/Frameworks/Security.framework" \
		-DSECURITY_LDFLAGS="-framework Security" \
		-DCOREFOUNDATION_FOUND=TRUE \
		-DCOREFOUNDATION_LIBRARIES="${SDK_DIR}/System/Library/Frameworks/CoreFoundation.framework" \
		-DCOREFOUNDATION_LDFLAGS="-framework CoreFoundation" \
		-DCMAKE_C_FLAGS="-Wno-error"
	make && make install
	# patch away pkg-config dependency on zlib (it's in the SDK)
	perl -pi -e 's/^(Requires.private:.*)zlib(.*)$/$1 $2/' "${PKG_CONFIG_PATH}/libgit2.pc"
	cd "${PARENT_DIR}"
fi

echo "iOS native libraries built successfully in ${PREFIX}"
