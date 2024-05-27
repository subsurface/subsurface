# Tool repo to crosscompile subsurface for iOS

Dependencies:

- This only works on a Mac
- XCode with iOS SDK and Qt5.13 or later
- cmake

Follow [these instructions](/INSTALL.md#cross-building-subsurface-on-macosx-for-ios)
and then continue here:

1. `cd <repo>/packaging/ios`
2. `export IOS_BUNDLE_PRODUCT_IDENTIFIER="<your apple id>.subsurface-divelog.subsurface-mobile"`
3. `./build.sh`
note: this builds all dependencies and is only needed first time
      it currently build for armv7 arm64 and x86_64 (simulator)

1. `cd <repo>/..`
2. Launch QtCreator and open `subsurface/packaging/ios/Subsurface-mobile.pro`
3. Build Subsurface-mobile in QtCreator - you can build for the simulator and for
a device and even deploy to a connected device.

Everything up to here you can do without paying for an Apple Developer account.

In order to create a bundle that can be distributed things get even more
complex and an Apple Developer account definitely is necessary in order for you
to be able to sign the bundle.

The easiest way to do that appears to be to open the Subsurface-mobile.xcodeproj
in the build directory that QtCreator used in Xcode and to create an archive there.


**WARNING:**

The version number used in the Subsurface-mobile app is created in step 3.
So whenever you pull the latest git or commit a change, you need to re-run the
`build.sh` script so that the `Info.plist` used by QtCreator (well, by Xcode under
the hood) gets updated. Otherwise you will continue to see the old version
number, even though the sources have been recompiled which can be very
confusing.

Do a simply version update by running:
```
build.sh -version
```
and then rebuilding in Qt Creator (or Xcode).
