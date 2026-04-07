# Building Subsurface-mobile for Android

Use the pre-built Docker image that contains the NDK, SDK, Qt, and all pre-compiled native dependencies.

Initial setup:

```bash
cd /some/path
git clone https://github.com/subsurface/subsurface
cd subsurface
git submodule init
git submodule update --recursive
```

Now use `local-build.sh` in order to run builds inside the container. This
mounts the Subsurface source tree as well as the build output trees as
volumes, ensuring that builds are fully incremental and build artifacts are
easy to analyze.
The build artifacts will be in `/some/path/build-android`,
`/some/path/kirigami-build-android`, `/some/path/googlemaps-build-android`
and the  output (.apk and if requested, .aab) will be in
`/some/path/subsurface/output/android`.

If you provide a `.secrets` file in the Subsurface source directory (or a
location specified in the -secrets argument) with the encoded signing key,
the outputs will be signed. The format of this file is
```
ANDROID_KEYSTORE_BASE64=<base64 encoded key>
ANDROID_KEYSTORE_PASSWORD=<password>
ANDROID_KEYSTORE_ALIAS=<keystore alias used>
```

```bash
bash packaging/android/local-build.sh [-secrets <path to secrets file>] [-build-aab]
```

Note: if you test your own build on a device, since you do not have the
release  signing key, you need to uninstall any official Subsurface-mobile
build before  installing your own, and vice versa.

## CI workflow

The GitHub Actions workflow (`.github/workflows/android.yml`) runs the same
Docker-based build, signs the APK/AAB with the project keystore, and uploads
the artifacts. On pushes to `master` it also creates a nightly release.

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
4. `local-build.sh` (and the CI workflow) then run Gradle to produce the
APK/AAB and sign them with the keystore.
