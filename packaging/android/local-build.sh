#!/bin/bash
#
# Build, sign, and package Subsurface-mobile for Android using the Docker container.
# This is the local equivalent of the GitHub Actions workflow in .github/workflows/android.yml.
#
# Prerequisites:
#   - Docker or Podman
#   - A .secrets file in the repository root containing:
#       ANDROID_KEYSTORE_BASE64=<base64-encoded keystore>
#       ANDROID_KEYSTORE_PASSWORD=<keystore password>
#       ANDROID_KEYSTORE_ALIAS=<key alias>
#
# The build-android and kirigami-build directories are mounted from the host
# (as siblings of the source tree) so that incremental rebuilds are fast.
#
# Optionally, set ANDROID_APPLICATION_ID in .secrets to override the
# default applicationId (org.subsurfacedivelog.mobile), e.g. for a
# sponsor build.
#
# Usage:
#   cd <repo>
#   packaging/android/local-build.sh [path/to/.secrets]

set -e

SUBSURFACE_SOURCE="$(cd "$(dirname "$0")/../../" && pwd)"
cd "${SUBSURFACE_SOURCE}"

SECRETS_FILE="${1:-${SUBSURFACE_SOURCE}/.secrets}"
if [ ! -f "${SECRETS_FILE}" ]; then
	echo "Error: ${SECRETS_FILE} not found."
	echo "Create it with ANDROID_KEYSTORE_BASE64, ANDROID_KEYSTORE_PASSWORD, and ANDROID_KEYSTORE_ALIAS."
	exit 1
fi

# load secrets
# shellcheck disable=SC1090
source "${SECRETS_FILE}"

for var in ANDROID_KEYSTORE_BASE64 ANDROID_KEYSTORE_PASSWORD ANDROID_KEYSTORE_ALIAS; do
	if [ -z "${!var}" ]; then
		echo "Error: ${var} not set in ${SECRETS_FILE}"
		exit 1
	fi
done

# compute version information
VERSION=$(scripts/get-version.sh)
VERSION_4=$(scripts/get-version.sh 4)
BUILDNR=$(scripts/get-version.sh 1)

echo "=== Building Subsurface-mobile ${VERSION} ==="

# detect container runtime: prefer podman if available, fall back to docker
if command -v podman &>/dev/null; then
	CONTAINER_RT=podman
elif command -v docker &>/dev/null; then
	CONTAINER_RT=docker
else
	echo "Error: neither podman nor docker found in PATH"
	exit 1
fi
echo "Using container runtime: ${CONTAINER_RT}"

CONTAINER_IMAGE="docker.io/subsurface/android-build:6.10.0"

# These must match the values in
# scripts/docker/android-build-container/Dockerfile
SDK_VERSION="35.0.0"
ANDROID_SDK_ROOT="/opt/android-sdk"
BUILDROOT="/android"

# decode the keystore to a temporary file
KEYSTORE_FILE=$(mktemp)
trap 'rm -f "${KEYSTORE_FILE}"' EXIT
echo "${ANDROID_KEYSTORE_BASE64}" | base64 -d > "${KEYSTORE_FILE}"

# persistent build directories on the host for incremental rebuilds
BUILD_ANDROID="${SUBSURFACE_SOURCE}/../build-android"
KIRIGAMI_BUILD="${SUBSURFACE_SOURCE}/../kirigami-build-android"
GOOGLEMAPS_BUILD="${SUBSURFACE_SOURCE}/../googlemaps-build-android"
OUTPUT_DIR="${SUBSURFACE_SOURCE}/output/android"
mkdir -p "${BUILD_ANDROID}" "${KIRIGAMI_BUILD}" "${GOOGLEMAPS_BUILD}" "${OUTPUT_DIR}"

# build, package, and sign everything in a single container
${CONTAINER_RT} run --rm \
	-v "${SUBSURFACE_SOURCE}:${BUILDROOT}/src/subsurface" \
	-v "${BUILD_ANDROID}:${BUILDROOT}/build-android" \
	-v "${KIRIGAMI_BUILD}:${BUILDROOT}/src/kirigami-build" \
	-v "${GOOGLEMAPS_BUILD}:${BUILDROOT}/src/googlemaps-build" \
	-v "${OUTPUT_DIR}:${BUILDROOT}/output" \
	-v "${KEYSTORE_FILE}:/tmp/keystore:ro" \
	-e BUILDNR="${BUILDNR}" \
	-e VERSION="${VERSION}" \
	-e VERSION_4="${VERSION_4}" \
	-e HOST_UID="$(id -u)" \
	-e HOST_GID="$(id -g)" \
	-e KS_PASS="${ANDROID_KEYSTORE_PASSWORD}" \
	-e KS_ALIAS="${ANDROID_KEYSTORE_ALIAS}" \
	-e APP_ID="${ANDROID_APPLICATION_ID:-}" \
	"${CONTAINER_IMAGE}" \
	bash -c "
		set -e
		bash -x ${BUILDROOT}/src/subsurface/scripts/docker/android-build-container/android-build-subsurface.sh

		echo '=== Building AAB ==='
		cd ${BUILDROOT}/build-android/android-build
		GRADLE_PROPS=\"\"
		if [ -n \"\${APP_ID}\" ]; then
			GRADLE_PROPS=\"-PsubsurfaceApplicationId=\${APP_ID}\"
		fi
		./gradlew bundleRelease \${GRADLE_PROPS}

		echo '=== Collecting artifacts ==='
		APK=\$(find ${BUILDROOT}/build-android/android-build -name '*.apk' | head -1)
		AAB=\$(find ${BUILDROOT}/build-android/android-build -name '*.aab' | head -1)
		cp \"\${APK}\" ${BUILDROOT}/output/Subsurface-mobile-${VERSION}.apk
		cp \"\${AAB}\" ${BUILDROOT}/output/Subsurface-mobile-${VERSION}.aab

		echo '=== Signing ==='
		BT=${ANDROID_SDK_ROOT}/build-tools/${SDK_VERSION}
		APK=${BUILDROOT}/output/Subsurface-mobile-${VERSION}.apk
		AAB=${BUILDROOT}/output/Subsurface-mobile-${VERSION}.aab

		\${BT}/zipalign -P 16 4 \${APK} /tmp/aligned.apk
		\${BT}/apksigner sign \
			--ks /tmp/keystore \
			--ks-pass \"pass:\${KS_PASS}\" \
			--ks-key-alias \"\${KS_ALIAS}\" \
			--in /tmp/aligned.apk \
			--out \${APK}

		jarsigner -sigalg SHA256withRSA -digestalg SHA-256 \
			-keystore /tmp/keystore \
			-storepass \"\${KS_PASS}\" \
			\${AAB} \
			\"\${KS_ALIAS}\"

		chown \${HOST_UID}:\${HOST_GID} ${BUILDROOT}/output/*
	"

echo "=== Done ==="
echo "Signed APK: ${OUTPUT_DIR}/Subsurface-mobile-${VERSION}.apk"
echo "Signed AAB: ${OUTPUT_DIR}/Subsurface-mobile-${VERSION}.aab"
