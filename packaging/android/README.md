# Building Subsurface-mobile for Android

## Using the Docker container (recommended)

The easiest way to build is to use the pre-built Docker image that contains the
NDK, SDK, Qt, and all pre-compiled native dependencies.

```sh
cd /some/path
git clone https://github.com/subsurface/subsurface
cd subsurface
git submodule init
git submodule update --recursive
```

Now use `android-build-subsurface.sh` inside the container. Mount the Subsurface source tree as a volume:

```bash
docker run --rm \
    -v /path/to/subsurface:/android/src/subsurface \
    -e BUILDNR=1234 \
    -e VERSION="6.0.5000" \
    -e VERSION_4="6.0.5000.3" \
    android-build-container \
    bash /android/src/subsurface/scripts/docker/android-build-container/android-build-subsurface.sh
```

After a successful build, the APK is at:
```
/android/build-android/android-build/build/outputs/apk/release/android-build-release-unsigned.apk
```

To extract it from the container, either mount an output directory or use `docker cp`.

Note: since you do not have the release signing key, you need to uninstall
any official Subsurface-mobile build before installing your own, and vice
versa.

## CI workflow

The GitHub Actions workflow (`.github/workflows/android.yml`) runs the same
Docker-based build, signs the APK/AAB with the project keystore, and uploads
the artifacts. On pushes to `master` it also publishes to the Google Play
Store and creates a nightly release.

## Docker image

The container image is defined in
`scripts/docker/android-build-container/Dockerfile`. It is a multi-stage
build:

1. **base** -- Ubuntu 24.04 with NDK 27.2, SDK 35, JDK 17, cmake, ninja
2. **lib-base** -- builds native libraries (OpenSSL, libxml2, libxslt,
   libzip, libgit2, SQLite) into `install-root-arm64-v8a`
3. **final** -- copies the pre-built libraries and installs Qt
   (Android arm64-v8a + host tools)

The image is rebuilt via `.github/workflows/android-dockerimage.yml` and
pushed to Docker Hub as `subsurface/android-build:<tag>`.

## Build script overview

`scripts/docker/android-build-container/android-build-subsurface.sh` runs
inside the container and performs these steps:

1. Build Kirigami and ECM (via `scripts/mobilecomponents.sh`)
2. Cross-compile libdivecomputer
3. Configure and build Subsurface with cmake + Ninja
4. The caller (CI or you) then runs Gradle to produce the APK/AAB
