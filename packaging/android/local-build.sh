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
# A named container ("subsurface-android-build") is kept between runs so
# that build artifacts inside the container persist and incremental rebuilds
# are fast. Only the source tree and the output directory are bind-mounted
# from the host; everything else lives inside the container's writable
# layer.
#
# When the container image changes (e.g. a new Qt or NDK version), the old
# container is automatically removed and recreated from the new image.
#
# To force a full clean rebuild, remove the container:
#   docker rm -f subsurface-android-build  # or podman
#
# Optionally, set ANDROID_APPLICATION_ID in .secrets to override the
# default applicationId (org.subsurfacedivelog.mobile), e.g. for a
# sponsor build.
#
# Usage:
#   cd <repo>
#   packaging/android/local-build.sh [options] [path/to/.secrets]
#
# Options:
#   -debug             Build debug APK (allows version downgrade, smaller without symbols)
#   -release           Build release APK (default; no version downgrade allowed)
#   -build-aab         Also build Android App Bundle (.aab) in addition to APK
#   -secrets <file>    Path to .secrets file (default: ./.secrets)

set -e

SUBSURFACE_SOURCE="$(cd "$(dirname "$0")/../../" && pwd)"
cd "${SUBSURFACE_SOURCE}"

SECRETS_FILE="${SUBSURFACE_SOURCE}/.secrets"
BUILD_AAB="0"
BUILD_TYPE="release"

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
		-debug)
			BUILD_TYPE="debug"
			;;
		-release)
			BUILD_TYPE="release"
			;;
		*)
			echo "Unknown command line argument $arg"
			echo "Usage: ${BASH_SOURCE[0]} [-secrets <secrets filename>] [-build-aab] [-debug | -release]"
			exit 1
			;;
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
CONTAINER_NAME="subsurface-android-build"

# Must match the BUILDROOT value in
# scripts/docker/android-build-container/Dockerfile
BUILDROOT="/android"

OUTPUT_DIR="${SUBSURFACE_SOURCE}/output/android"
mkdir -p "${OUTPUT_DIR}"

# --- Container lifecycle management ---
#
# We keep a named container around between runs so that all build state
# (build-android, kirigami-build, googlemaps, libdivecomputer-build, and
# the install-root with its pre-compiled native libraries) persists in the
# container's writable layer. Only the source tree and output directory
# are bind-mounted from the host.
#
# If the container image has changed, we remove the old container and
# recreate it from the new image.

# Make sure the image is present locally so we can read its ID.
IMAGE_ID=$(${CONTAINER_RT} image inspect --format '{{.Id}}' "${CONTAINER_IMAGE}" 2>/dev/null || true)
if [ -z "${IMAGE_ID}" ]; then
	echo "Pulling ${CONTAINER_IMAGE}..."
	${CONTAINER_RT} pull "${CONTAINER_IMAGE}"
	IMAGE_ID=$(${CONTAINER_RT} image inspect --format '{{.Id}}' "${CONTAINER_IMAGE}")
fi

# Check if the container already exists and whether it was created from
# the current image. If the image has changed, remove the stale container
# so it gets recreated below.
EXISTING_IMAGE=$(${CONTAINER_RT} inspect --format '{{.Image}}' "${CONTAINER_NAME}" 2>/dev/null || true)
if [ -n "${EXISTING_IMAGE}" ] && [ "${EXISTING_IMAGE}" != "${IMAGE_ID}" ]; then
	echo "Container image changed -- removing old container for a clean rebuild"
	${CONTAINER_RT} rm -f "${CONTAINER_NAME}" >/dev/null
	EXISTING_IMAGE=""
fi

# Create the container if it doesn't exist. The container runs
# "sleep infinity" as its main process so we can docker-start / docker-exec
# it repeatedly. Only the source tree and output directory are
# bind-mounted from the host; all build state lives inside the container.
if [ -z "${EXISTING_IMAGE}" ]; then
	echo "Creating build container ${CONTAINER_NAME}..."
	${CONTAINER_RT} create \
		--name "${CONTAINER_NAME}" \
		-v "${SUBSURFACE_SOURCE}:${BUILDROOT}/src/subsurface" \
		-v "${OUTPUT_DIR}:${BUILDROOT}/output" \
		"${CONTAINER_IMAGE}" \
		sleep infinity >/dev/null
fi

# Start the container (no-op if already running).
${CONTAINER_RT} start "${CONTAINER_NAME}" >/dev/null

# Copy the keystore into the running container.
${CONTAINER_RT} cp "${KEYSTORE_FILE}" "${CONTAINER_NAME}:/tmp/keystore"

# --- Run the build ---
# All build logic (compile, strip, gradle, collect artifacts, sign, chown)
# lives in android-build-subsurface.sh. Running it directly avoids fragile
# nested quoting that bash -c "..." requires.
${CONTAINER_RT} exec \
	-e BUILDNR="${BUILDNR}" \
	-e VERSION="${VERSION}" \
	-e VERSION_4="${VERSION_4}" \
	-e HOST_UID="$(id -u)" \
	-e HOST_GID="$(id -g)" \
	-e KS_PASS="${ANDROID_KEYSTORE_PASSWORD}" \
	-e KS_ALIAS="${ANDROID_KEYSTORE_ALIAS}" \
	-e APP_ID="${ANDROID_APPLICATION_ID:-}" \
	-e BUILD_AAB="${BUILD_AAB}" \
	-e BUILD_TYPE="${BUILD_TYPE}" \
	"${CONTAINER_NAME}" \
	bash -ex "${BUILDROOT}/src/subsurface/scripts/docker/android-build-container/android-build-subsurface.sh"

# Stop the container (keeps it around for the next incremental build).
${CONTAINER_RT} stop "${CONTAINER_NAME}" >/dev/null

echo "=== Done ==="
if [ -n "$ANDROID_KEYSTORE_ALIAS" ]; then
	echo "Signed APK: ${OUTPUT_DIR}/Subsurface-mobile-${VERSION}.apk"
	if [ "$BUILD_AAB" = "1" ]; then
		echo "Signed AAB: ${OUTPUT_DIR}/Subsurface-mobile-${VERSION}.aab"
	fi
fi
