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

SECRETS_FILE="${SUBSURFACE_SOURCE}/.secrets"
BUILD_AAB="0"

while [ $# -gt 0 ]; do
	arg="$1"
	case $arg in
		-secrets)
			shift
			SECRETS_FILE="$1"
			;;
		-build-aab)
			BUILD_AAB="1"
			;;
		*)
			echo "Unknown command line argument $arg"
			echo "Usage: ${BASH_SOURCE[0]} [-secrets <secrets filename>] [-build-aab]"
	esac
	shift
done

if [ ! -f "${SECRETS_FILE}" ]; then
	echo "No ${SECRETS_FILE} found, not signing output."
	ANDROID_KEYSTORE_BASE64=""
	ANDROID_KEYSTORE_PASSWORD=""
	ANDROID_KEYSTORE_ALIAS=""
	KEYSTORE_FILE="/dev/null"
else
	# load secrets -- if we have a secrets file, it needs to provide all values
	# shellcheck disable=SC1090
	source "${SECRETS_FILE}"

	for var in ANDROID_KEYSTORE_BASE64 ANDROID_KEYSTORE_PASSWORD ANDROID_KEYSTORE_ALIAS; do
		if [ -z "${!var}" ]; then
			echo "Error: ${var} not set in ${SECRETS_FILE}"
			exit 1
		fi
	done
	# decode the keystore to a temporary file
	KEYSTORE_FILE=$(mktemp)
	trap 'rm -f "${KEYSTORE_FILE}"' EXIT
	echo "${ANDROID_KEYSTORE_BASE64}" | base64 -d > "${KEYSTORE_FILE}"
fi

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

CONTAINER_IMAGE="docker.io/subsurface/android-build:6.10.3-2"

# These must match the values in
# scripts/docker/android-build-container/Dockerfile
SDK_VERSION="35.0.0"
ANDROID_SDK_ROOT="/opt/android-sdk"
BUILDROOT="/android"

# persistent build directories on the host for incremental rebuilds.
# Resolve to canonical absolute paths (no '..') -- podman with some storage
# drivers walks each path component and statfs()es siblings during bind-mount
# setup, which can fail in confusing ways if ".." appears in the path.
PARENT_DIR="$(cd "${SUBSURFACE_SOURCE}/.." && pwd)"
BUILD_ANDROID="${PARENT_DIR}/build-android"
KIRIGAMI_BUILD="${PARENT_DIR}/kirigami-build-android"
GOOGLEMAPS_BUILD="${PARENT_DIR}/googlemaps-build-android"
GOOGLEMAPS_SRC="${PARENT_DIR}/googlemaps-android"
LIBDC_BUILD="${PARENT_DIR}/libdivecomputer-build-android"
INSTALL_ROOT="${PARENT_DIR}/install-root-arm64-v8a-android"
OUTPUT_DIR="${SUBSURFACE_SOURCE}/output/android"

mkdir -p "${BUILD_ANDROID}" "${KIRIGAMI_BUILD}" "${GOOGLEMAPS_BUILD}" \
	"${GOOGLEMAPS_SRC}" "${LIBDC_BUILD}" "${INSTALL_ROOT}" "${OUTPUT_DIR}"

# Stamp file recording which container image (by content-addressed ID) the
# persistent build directories were last populated from. If the image changes
# we throw all of them away and start clean -- a new image may ship a new Qt
# or NDK, and stale build trees pinned to the old toolchain paths would fail
# in confusing ways.
STAMP="${INSTALL_ROOT}/.image-id"

# Make sure the image is present locally so we can read its ID.
IMAGE_ID=$(${CONTAINER_RT} image inspect --format '{{.Id}}' "${CONTAINER_IMAGE}" 2>/dev/null || true)
if [ -z "${IMAGE_ID}" ]; then
	echo "Pulling ${CONTAINER_IMAGE}..."
	${CONTAINER_RT} pull "${CONTAINER_IMAGE}"
	IMAGE_ID=$(${CONTAINER_RT} image inspect --format '{{.Id}}' "${CONTAINER_IMAGE}")
fi

if [ ! -f "${STAMP}" ] || [ "$(cat "${STAMP}" 2>/dev/null)" != "${IMAGE_ID}" ]; then
	if [ -f "${STAMP}" ]; then
		echo "Container image changed -- discarding stale build trees for a clean rebuild"
	else
		echo "First run -- seeding install-root from container image"
	fi
	# The wipe runs *inside* a container rather than on the host because the
	# files in these directories were created by previous container runs and
	# may be owned by root (under rootful docker on Linux). Removing them as
	# the host user would fail with EPERM. Inside a container we are root (or,
	# under rootless podman, the user-namespace mapping makes us effectively
	# the owner) and can clean up unconditionally. We mount the parent of the
	# source tree so a single rm -rf can reach all six directories.
	${CONTAINER_RT} run --rm \
		-v "${PARENT_DIR}:/parent" \
		"${CONTAINER_IMAGE}" \
		bash -c '
			rm -rf /parent/build-android \
			       /parent/kirigami-build-android \
			       /parent/googlemaps-build-android \
			       /parent/googlemaps-android \
			       /parent/libdivecomputer-build-android \
			       /parent/install-root-arm64-v8a-android
		'
	mkdir -p "${INSTALL_ROOT}"
	# Seed install-root and write the stamp from inside the container, so the
	# stamp file ends up with the same (root-owned, under rootful docker)
	# ownership as everything else in install-root. Writing the stamp from the
	# host would fail with EACCES under rootful docker because the host user
	# can't create files inside a directory whose contents the container just
	# populated as root.
	${CONTAINER_RT} run --rm \
		-e IMAGE_ID="${IMAGE_ID}" \
		-v "${INSTALL_ROOT}:/host-install-root" \
		"${CONTAINER_IMAGE}" \
		bash -c "
			cp -a /android/src/install-root-arm64-v8a/. /host-install-root/
			printf '%s\n' \"\${IMAGE_ID}\" > /host-install-root/.image-id
		"
	# The wipe container removed every directory we'll bind-mount into the
	# main build container below. Recreate the empty ones now -- if we don't,
	# podman fails to start the build container with a "statfs: no such file
	# or directory" on the first missing bind source.
	mkdir -p "${BUILD_ANDROID}" "${KIRIGAMI_BUILD}" "${GOOGLEMAPS_BUILD}" \
		"${GOOGLEMAPS_SRC}" "${LIBDC_BUILD}"
fi

# build, package, and sign everything in a single container
#
# NOTE: this says 'run --rm', but all the state is in the mounted volumes
#       so this works perfectly for incremental builds and the typical developer
#       workflows
${CONTAINER_RT} run --rm \
	-v "${SUBSURFACE_SOURCE}:${BUILDROOT}/src/subsurface" \
	-v "${BUILD_ANDROID}:${BUILDROOT}/build-android" \
	-v "${KIRIGAMI_BUILD}:${BUILDROOT}/src/kirigami-build" \
	-v "${GOOGLEMAPS_BUILD}:${BUILDROOT}/src/googlemaps-build" \
	-v "${GOOGLEMAPS_SRC}:${BUILDROOT}/src/googlemaps" \
	-v "${LIBDC_BUILD}:${BUILDROOT}/libdivecomputer-build" \
	-v "${INSTALL_ROOT}:${BUILDROOT}/src/install-root-arm64-v8a" \
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
	-e BUILD_AAB="${BUILD_AAB}" \
	"${CONTAINER_IMAGE}" \
	bash -c "
		set -e
		bash -x ${BUILDROOT}/src/subsurface/scripts/docker/android-build-container/android-build-subsurface.sh
		if [ $BUILD_AAB = 1 ]; then
			echo '=== Building AAB ==='
			cd ${BUILDROOT}/build-android/android-build
			GRADLE_PROPS=\"\"
			if [ -n \"\${APP_ID}\" ]; then
				GRADLE_PROPS=\"-PsubsurfaceApplicationId=\${APP_ID}\"
			fi
			./gradlew bundleRelease \${GRADLE_PROPS}
			AAB=\$(find ${BUILDROOT}/build-android/android-build -name '*.aab' | head -1)
			cp \"\${AAB}\" ${BUILDROOT}/output/Subsurface-mobile-${VERSION}.aab
			AAB=${BUILDROOT}/output/Subsurface-mobile-${VERSION}.aab
		fi
		echo '=== Collecting artifacts ==='
		APK=\$(find ${BUILDROOT}/build-android/android-build -name '*.apk' | head -1)
		cp \"\${APK}\" ${BUILDROOT}/output/Subsurface-mobile-${VERSION}.apk
		APK=${BUILDROOT}/output/Subsurface-mobile-${VERSION}.apk
		if [ -n \"\${KS_ALIAS}\" ]; then
			echo '=== Signing ==='
			BT=${ANDROID_SDK_ROOT}/build-tools/${SDK_VERSION}

			\${BT}/zipalign -P 16 4 \${APK} /tmp/aligned.apk
			\${BT}/apksigner sign \
				--ks /tmp/keystore \
				--ks-pass \"pass:\${KS_PASS}\" \
				--ks-key-alias \"\${KS_ALIAS}\" \
				--in /tmp/aligned.apk \
				--out \${APK}
			if [ $BUILD_AAB = 1 ]; then
				jarsigner -sigalg SHA256withRSA -digestalg SHA-256 \
					-keystore /tmp/keystore \
					-storepass \"\${KS_PASS}\" \
					\${AAB} \
					\"\${KS_ALIAS}\"
			fi
		fi
		chown \${HOST_UID}:\${HOST_GID} ${BUILDROOT}/output/*
	"

echo "=== Done ==="
if [ -n "$ANDROID_KEYSTORE_ALIAS" ]; then
	echo "Signed APK: ${OUTPUT_DIR}/Subsurface-mobile-${VERSION}.apk"
	if [ "$BUILD_AAB" = "1" ]; then
		echo "Signed AAB: ${OUTPUT_DIR}/Subsurface-mobile-${VERSION}.aab"
	fi
fi
