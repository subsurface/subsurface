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

docker run --rm -v "$PWD:/android/src/subsurface" \
    subsurface/android-build:6.8.1 \
    bash -x /android/src/subsurface/scripts/docker/android-build-container/android-build-subsurface.sh
```

You need to export `BUILDNR`, `VERSION`, and `VERSION_4` before the build
(the CI workflow computes these from the git history).

The build produces `libsubsurface-mobile.so` and the Gradle project under
`/android/src/subsurface/build-android/android-build/`. To create an APK:

```sh
cd build-android/android-build
./gradlew assembleDebug
```

The resulting APK is in `build/outputs/apk/debug/` and can be installed with:

```sh
adb install build/outputs/apk/debug/android-build-debug.apk
```

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
3. **final** -- copies the pre-built libraries and installs Qt 6.8.3
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
