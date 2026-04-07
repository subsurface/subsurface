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
| `ANDROID_PLATFORM` | Minimum API level to target | `28` |
| `ANDROID_BUILD_ABI` | Target ABI | `arm64-v8a` |
| `QT_VERSION` | Qt version | `6.10.3` |
| `BUILDROOT` | Working directory inside container | `/android` |
| `ANDROID_SDK_ROOT` | Android SDK install path | `/opt/android-sdk` |
