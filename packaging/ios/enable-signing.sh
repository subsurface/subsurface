#!/bin/bash
# AI-generated (Claude)
#
# Enable code signing on an existing iOS build directory.
# Run this after ios-build-subsurface.sh to configure signing,
# then rebuild with: cmake --build . --config Release
#
# Usage: packaging/ios/enable-signing.sh [build-dir]
#
# The build directory defaults to ../build-ios (relative to the
# subsurface source tree, matching ios-build-subsurface.sh).
#
# Environment variables:
#   DEVELOPMENT_TEAM  - Apple Development Team ID (default: A259RV63B3)
#   BUNDLE_ID         - Bundle identifier (default: org.subsurface-divelog.subsurface-mobile)

set -e

pushd "$(dirname "$0")/../../" > /dev/null
SUBSURFACE_SOURCE="$PWD"
popd > /dev/null
PARENT_DIR="$(cd "${SUBSURFACE_SOURCE}/.."; pwd)"

BUILD_DIR="${1:-${PARENT_DIR}/build-ios}"

if [ ! -d "${BUILD_DIR}" ]; then
	echo "Error: build directory '${BUILD_DIR}' does not exist."
	echo "Run ios-build-subsurface.sh first."
	exit 1
fi

DEVELOPMENT_TEAM="${DEVELOPMENT_TEAM:-A259RV63B3}"
BUNDLE_ID="${BUNDLE_ID:-org.subsurface-divelog.subsurface-mobile}"

cd "${BUILD_DIR}"

echo "Enabling code signing in ${BUILD_DIR}"
echo "  Team: ${DEVELOPMENT_TEAM}"
echo "  Bundle ID: ${BUNDLE_ID}"

# Re-run cmake with signing enabled
cmake "${SUBSURFACE_SOURCE}" \
	-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=YES \
	-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY="Apple Development" \
	-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="${DEVELOPMENT_TEAM}" \
	-DCMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER="${BUNDLE_ID}"

echo ""
echo "Signing enabled. Now rebuild with:"
echo "  cd ${BUILD_DIR}"
echo "  cmake --build . --config Release"
echo ""
echo "Or open the Xcode project to archive for distribution:"
echo "  open ${BUILD_DIR}/Subsurface-mobile.xcodeproj"
