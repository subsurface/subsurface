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

The following directories are created as siblings of the source tree and
persist between runs so that ninja, cmake and the autotools-based subbuilds
can do incremental work:

- `/some/path/build-android` -- main Subsurface cmake/ninja build tree
- `/some/path/kirigami-build-android` -- Kirigami build tree
- `/some/path/googlemaps-android` -- googlemaps source checkout
- `/some/path/googlemaps-build-android` -- googlemaps Qt plugin build tree
- `/some/path/libdivecomputer-build-android` -- libdivecomputer build tree
- `/some/path/install-root-arm64-v8a-android` -- install prefix for the
  cross-compiled native libraries (OpenSSL, SQLite, libxml2, libxslt, libzip,
  libgit2, libdivecomputer, Kirigami, googlemaps); seeded on first run from
  the contents baked into the container image

The signed output (.apk and if requested, .aab) ends up in
`/some/path/subsurface/output/android`.

### Container image changes and clean rebuilds

When the container image is upgraded -- e.g. because a new Qt or NDK version
ships -- the existing build trees and install root would be pinned to
toolchain paths from the old image and would fail in confusing ways. To
avoid this, `local-build.sh` records the content-addressed ID of the image
it last used in `install-root-arm64-v8a-android/.image-id`. On every run it
compares the stamp against the ID of the current image and, if they differ,
wipes all six directories above and re-seeds the install root from the new
image before building. The wipe is done inside a throwaway container so it
works regardless of which user owns the files on the host.

If you ever want to force a full clean rebuild without changing the image,
just delete the stamp file:

```bash
rm /some/path/install-root-arm64-v8a-android/.image-id
```

The next `local-build.sh` invocation will treat it as a fresh start.

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
