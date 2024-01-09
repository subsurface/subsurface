
None of these artifacts are signed with an official key.

# Android
The Android APK (named **Subsurface-mobile-\<version\>-CICD-release.apk** below) can be side-loaded on most Android devices. *Note*: If you previously had installed Subsurface-mobile from the Google Play store, the first time you switch to a CICD-release, you'll have to uninstall the Play store version first. Make sure your data is synced to the Subsurface Cloud. After you install the CICD-release and log in to the Subsurface Cloud again, the data will be restored. This is *only* required the first time you switch from a Play store release to a CICD-release.

# Windows
The Windows installer (named **subsurface-\<version\>-CICD-release-installer.exe **) will ask you to confirm installation of an app from an unknown developer. The other **Windows .exe** files are only useful for a few users who are specifically directed to try them. Unless that is you, you typically will want to try the **...-installer.exe** file.

# macOS
The macOS DMG (named ** Subsurface-\<version\>-CICD-release.dmg) makes it even harder with a multi-step dance that requires opening the Privacy & Security settings in the System Preferences and explicitly confirming that you are willing to install this app.

# Linux
You can find similar Subsurface-Daily builds for [Ubuntu](https://ppa.launchpadcontent.net/subsurface) and Subsurface-test for [Fedora](https://copr.fedorainfracloud.org/coprs/dirkhh/Subsurface-test).

Finally, there is a Linux AppImage (named **Subsurface-\<version\>-CICD-release.AppImage**) should run on most other x86_64 Linux distributions for which we don't have specific builds.

**Note:** Due to the asynchronous nature of our build process the artifacts for the individual platforms are added to the release one by one, whenever their build is finished. So if you can not find a particular artifact on a recent (less than 2 hours old) build, please come back later and check again.

Please report any issues with these builds in the [Subsurface user forum](https://groups.google.com/g/subsurface-divelog).
