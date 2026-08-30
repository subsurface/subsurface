#!/bin/bash
# AI-generated (Claude)
#
# Helper for the S03 on-device Export/Share FIT UAT.
#
# Subcommands:
#   check    Verify a fresh, signed, current-version APK exists in output/android/.
#   install  Run check, then install that APK onto a connected device (update install,
#            never uninstalls).
#
# Usage:
#   packaging/android/fit-uat-device.sh check
#   packaging/android/fit-uat-device.sh install
set -euo pipefail

ANDROID_PACKAGE="org.subsurfacedivelog.mobile"

SUBSURFACE_SOURCE="$(cd "$(dirname "$0")/../../" && pwd)"
cd "${SUBSURFACE_SOURCE}"

usage() {
	echo "Usage: $(basename "$0") <check|install>"
}

cmd_check() {
	VERSION="$(scripts/get-version.sh)"
	APK="output/android/Subsurface-mobile-${VERSION}.apk"

	if [ ! -f "${APK}" ]; then
		echo "Error: expected APK not found: ${APK}" >&2
		echo "Build it first with: packaging/android/local-build.sh [-debug]" >&2
		exit 1
	fi

	if ! grep -aq 'APK Sig Block 42' "${APK}"; then
		echo "Error: ${APK} is not signed (missing APK Signing Block magic 'APK Sig Block 42')" >&2
		exit 1
	fi

	# Capture the listing into a variable rather than piping straight into
	# grep -q: under pipefail, grep -q's early exit on first match sends
	# unzip a SIGPIPE, and pipefail then reports unzip's non-zero signal
	# exit even though grep matched.
	LISTING="$(unzip -l "${APK}")"

	if ! grep -q 'classes\.dex' <<<"${LISTING}"; then
		echo "Error: ${APK} is missing classes.dex" >&2
		exit 1
	fi

	if ! grep -q 'lib/arm64-v8a/libsubsurface-mobile_arm64-v8a\.so' <<<"${LISTING}"; then
		echo "Error: ${APK} is missing lib/arm64-v8a/libsubsurface-mobile_arm64-v8a.so" >&2
		exit 1
	fi

	echo "${APK}"
}

cmd_install() {
	# Reuse check's VERSION/APK resolution so a stale or unsigned APK is
	# never installed. cmd_check runs in a command-substitution subshell,
	# so recompute VERSION here too (same deterministic git-derived value)
	# rather than relying on subshell-local state leaking out.
	APK="$(cmd_check)"
	VERSION="$(scripts/get-version.sh)"

	DEVICE_LINE="$(adb devices | awk 'NR>1 && $2=="device" {print; exit}')"
	if [ -z "${DEVICE_LINE}" ]; then
		echo "Error: no device in 'device' state. Full 'adb devices' output:" >&2
		ALL_DEVICES="$(adb devices)"
		echo "${ALL_DEVICES}" >&2
		if grep -q 'unauthorized' <<<"${ALL_DEVICES}"; then
			echo "A device is 'unauthorized' - accept the on-phone USB-debugging RSA prompt." >&2
		fi
		if grep -q 'offline' <<<"${ALL_DEVICES}"; then
			echo "A device is 'offline' - reconnect the USB cable and retry." >&2
		fi
		exit 1
	fi
	SERIAL="$(awk '{print $1}' <<<"${DEVICE_LINE}")"

	echo "Installing ${APK} on device ${SERIAL}..."
	INSTALL_OUTPUT="$(adb -s "${SERIAL}" install -r "${APK}" 2>&1)" || {
		echo "Error: adb install failed:" >&2
		echo "${INSTALL_OUTPUT}" >&2
		exit 1
	}
	if ! grep -q '^Success' <<<"${INSTALL_OUTPUT}"; then
		echo "Error: adb install did not report Success:" >&2
		echo "${INSTALL_OUTPUT}" >&2
		exit 1
	fi

	DUMPSYS_VERSION_LINE="$(adb -s "${SERIAL}" shell dumpsys package "${ANDROID_PACKAGE}" | grep -m1 'versionName=' || true)"
	INSTALLED_VERSION="$(sed -e 's/^.*versionName=//' -e 's/[[:space:]]*$//' <<<"${DUMPSYS_VERSION_LINE}")"
	if [ "${INSTALLED_VERSION}" != "${VERSION}" ]; then
		echo "Error: installed versionName '${INSTALLED_VERSION}' does not match expected '${VERSION}'" >&2
		exit 1
	fi

	echo "Installed ${APK} (versionName=${INSTALLED_VERSION}) on device ${SERIAL}"
}

if [ $# -lt 1 ]; then
	usage
	exit 2
fi

case "$1" in
	check)
		cmd_check
		;;
	install)
		cmd_install
		;;
	*)
		usage
		exit 2
		;;
esac
