# Android Build Container

Docker-based build environment for cross-compiling Subsurface-mobile for Android (arm64-v8a).

## Prerequisites

- Docker (with BuildKit support, i.e. `docker buildx`)
- Enough disk space (~15 GB for the full container image)
- The Subsurface source tree checked out with submodules

## Building the Container

From this directory (`scripts/docker/android-build-container`):

```bash
# Copy get-dep-lib.sh into the build context (Docker can't access files outside it)
cp ../../get-dep-lib.sh .

# Build the container image
docker buildx build -t android-build-container .
```

This takes a while on the first run. It:
1. Installs build tools (cmake, ninja, JDK 17, etc.)
2. Downloads the Android SDK, NDK, and build tools
3. Cross-compiles native dependencies (libgit2, OpenSSL, libssh2, libxml2, etc.)
4. Installs Qt for both Android (cross-compilation target) and desktop Linux (host tools)

Subsequent builds are cached as long as the Dockerfile and the build scripts haven't changed.

### Configuration

All version pins and paths are defined as `ENV` variables at the top of the Dockerfile:

| Variable | Description | Default |
|----------|-------------|---------|
| `NDK_VERSION` | Android NDK version | `27.2.12479018` |
| `SDK_LEVEL` | Android SDK platform level | `35` |
| `SDK_VERSION` | Android build-tools version | `35.0.0` |
| `ANDROID_PLATFORM` | Minimum API level to target | `24` |
| `ANDROID_BUILD_ABI` | Target ABI | `arm64-v8a` |
| `QT_VERSION` | Qt version | `6.10.2` |
| `BUILDROOT` | Working directory inside container | `/android` |
| `ANDROID_SDK_ROOT` | Android SDK install path | `/opt/android-sdk` |

## Building Subsurface-mobile

Use `android-build-subsurface.sh` inside the container. Mount the Subsurface source tree as a volume:

```bash
docker run --rm \
    -v /path/to/subsurface:/android/src/subsurface \
    -e BUILDNR=1234 \
    -e VERSION="6.0.5000" \
    -e VERSION_4="6.0.5000.3" \
    android-build-container \
    bash /android/src/subsurface/scripts/docker/android-build-container/android-build-subsurface.sh
```

### Required Environment Variables

These must be passed to the container via `-e`:

| Variable | Description |
|----------|-------------|
| `BUILDNR` | Monotonically increasing build number (used for Google Play version code) |
| `VERSION` | Version string, e.g. `6.0.5000` |
| `VERSION_4` | Four-component version string, e.g. `6.0.5000.3` (typically from get-version.sh) |

### Output

After a successful build, the APK is at:
```
/android/build-android/android-build/build/outputs/apk/release/android-build-release-unsigned.apk
```

To extract it from the container, either mount an output directory or use `docker cp`.

### Version Codes

The Google Play Store requires a unique, monotonically increasing `versionCode` for each upload.
The build computes the version code as:

```
versionCode = BUILDNR * 10 + VERSION_CODE_OFFSET
```

`VERSION_CODE_OFFSET` (0-9) defaults to 0 and can be passed to cmake via `-DVERSION_CODE_OFFSET=n`
to allow resubmitting a fixed build without bumping the build number.

## File Overview

| File | Purpose |
|------|---------|
| `Dockerfile` | Multi-stage container definition |
| `android-build-subsurface.sh` | Main build script (runs inside container) |
| `android-native-libs.sh` | Cross-compiles native dependencies (libgit2, OpenSSL, etc.) |
| `setup-docker.sh` | Convenience wrapper for `docker buildx build` |
