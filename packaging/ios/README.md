# Building Subsurface-mobile for iOS

## Prerequisites

- macOS with Xcode and the iOS SDK installed
- Qt 6.8 or later (installed via the Qt online installer, with both the `ios`
  and `macos` kits and the following additional libraries: Qt 5 Compatibility module,
  Qt Connectivity, Qt Location (TP), Qt Positioning, and Qt Shader Tools)
- cmake, autoconf, automake, libtool, pkg-config (e.g. from Homebrew)

## Build

The `ios-build-subsurface.sh` script handles the entire build: native
dependencies, Kirigami, libdivecomputer, and Subsurface itself.

```
cd <repo>/packaging/ios
./ios-build-subsurface.sh
```

The script accepts several environment variables to override defaults:

| Variable | Default | Description |
|----------|---------|-------------|
| `QT_VERSION` | `6.10.2` | Qt version to use |
| `QT_ROOT` | `~/Qt` | Qt installation root |
| `ARCH` | `arm64` | Target architecture |
| `TARGET_SDK` | `iphoneos` | `iphoneos` or `iphonesimulator` |
| `BUILD_TYPE` | `Release` | `Release` or `Debug` |
| `BUILD_DIR` | `<repo>/../build` | Directory for all iOS dependency sources and build artefacts |

Example for a simulator build:

```
TARGET_SDK=iphonesimulator ARCH=arm64 ./ios-build-subsurface.sh
```

`BUILD_DIR` follows the desktop build convention: by default, it is a sibling
of the source checkout rather than a directory in it. To use a different
location, set it when invoking the script, for example:

```
BUILD_DIR="$HOME/subsurface-ios-build" ./ios-build-subsurface.sh
```

The resulting layout is:

```
<BUILD_DIR>/
  googlemaps/
  kirigami/                 # Kirigami, Breeze Icons and ECM sources
  kirigami-build/
  install-root/ios/<ARCH>/
  build-ios/subsurface-mobile.xcodeproj
```

The Xcode project and app bundle are under
`<BUILD_DIR>/build-ios/`.

**Incremental builds**

On a re-run with no source changes, `ios-build-subsurface.sh` skips rebuilding
Kirigami/ECM and the googlemaps plugin (it detects the installed outputs and
the recorded Kirigami version/patch-set marker). Native C libraries
(libxml2, libxslt, libzip, libgit2) and libdivecomputer are also skipped when
their installed artefacts are current.

To force a complete rebuild from scratch, delete or clear `BUILD_DIR` before
re-running. The `~/git/ios_build_subsurface.sh` wrapper supports a `rebuild`
argument that does this automatically.

**Note:** the Kirigami skip check covers both the pinned upstream version and
the set of local patches (`mobile-widgets/3rdparty/00*.patch`). Changing either
will trigger a full Kirigami rebuild on the next run.

## Signing and distribution

The build script disables code signing (`CODE_SIGNING_ALLOWED=NO`) so that it
works without an Apple Developer account. To sign and distribute:

1. Open `<BUILD_DIR>/build-ios/subsurface-mobile.xcodeproj` in Xcode.
2. Select the Subsurface-mobile target and configure your signing team and
   bundle identifier.
3. Build an archive from Xcode for distribution.

An Apple Developer account is required for distribution.
