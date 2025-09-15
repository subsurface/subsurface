
None of these artifacts are signed with an official key.

# Android
The Android APK (named **Subsurface-mobile-\<version\>-CICD-release.apk** below) can be side-loaded on most Android devices.
If you would like to get automatic updates, you can use [Obtainium](https://obtainium.imranr.dev/). After installing it, click on [this link](https://apps.obtainium.imranr.dev/redirect?r=obtainium://app/%7B%22id%22%3A%22org.subsurfacedivelog.mobile%22%2C%22url%22%3A%22https%3A%2F%2Fgithub.com%2Fsubsurface%2Fnightly-builds%22%2C%22author%22%3A%22subsurface%22%2C%22name%22%3A%22Subsurface-mobile%22%2C%22preferredApkIndex%22%3A0%2C%22additionalSettings%22%3A%22%7B%5C%22includePrereleases%5C%22%3Afalse%2C%5C%22fallbackToOlderReleases%5C%22%3Atrue%2C%5C%22filterReleaseTitlesByRegEx%5C%22%3A%5C%22%5C%22%2C%5C%22filterReleaseNotesByRegEx%5C%22%3A%5C%22%5C%22%2C%5C%22verifyLatestTag%5C%22%3Afalse%2C%5C%22sortMethodChoice%5C%22%3A%5C%22date%5C%22%2C%5C%22useLatestAssetDateAsReleaseDate%5C%22%3Afalse%2C%5C%22releaseTitleAsVersion%5C%22%3Afalse%2C%5C%22trackOnly%5C%22%3Afalse%2C%5C%22versionExtractionRegEx%5C%22%3A%5C%22v%28.%2A%29%5C%22%2C%5C%22matchGroupToUse%5C%22%3A%5C%22%241%5C%22%2C%5C%22versionDetection%5C%22%3Afalse%2C%5C%22releaseDateAsVersion%5C%22%3Afalse%2C%5C%22useVersionCodeAsOSVersion%5C%22%3Afalse%2C%5C%22apkFilterRegEx%5C%22%3A%5C%22%5C%22%2C%5C%22invertAPKFilter%5C%22%3Afalse%2C%5C%22autoApkFilterByArch%5C%22%3Atrue%2C%5C%22appName%5C%22%3A%5C%22%5C%22%2C%5C%22appAuthor%5C%22%3A%5C%22%5C%22%2C%5C%22shizukuPretendToBeGooglePlay%5C%22%3Afalse%2C%5C%22allowInsecure%5C%22%3Afalse%2C%5C%22exemptFromBackgroundUpdates%5C%22%3Afalse%2C%5C%22skipUpdateNotifications%5C%22%3Afalse%2C%5C%22about%5C%22%3A%5C%22This%20is%20the%20latest%20development%20release.%20For%20more%20info%20visit%20https%3A%2F%2Fsubsurface-divelog.org%2F.%5C%22%2C%5C%22refreshBeforeDownload%5C%22%3Afalse%7D%22%2C%22overrideSource%22%3A%22GitHub%22%7D) on your device to add the latest development release version of Subsurface-mobile to Obtainium, and get update notifications when new versions are released.
*Note*: If you previously had installed Subsurface-mobile from the Google Play store, the first time you switch to a CICD-release, you'll have to uninstall the Play store version first. Make sure your data is synced to the Subsurface Cloud. After you install the CICD-release and log in to the Subsurface Cloud again, the data will be restored. This is *only* required the first time you switch from a Play store release to a CICD-release.

# Windows
The Windows installer (named **subsurface-\<version\>-CICD-release-installer.exe **) will ask you to confirm installation of an app from an unknown developer. The other **Windows .exe** files are only useful for a few users who are specifically directed to try them. Unless that is you, you typically will want to try the **...-installer.exe** file.

# macOS
The macOS DMG (named ** Subsurface-\<version\>-CICD-release.dmg) makes it even harder with a multi-step dance that requires opening the Privacy & Security settings in the System Preferences and explicitly confirming that you are willing to install this app.

# Linux
You can find similar Subsurface-Daily builds for [Ubuntu](https://ppa.launchpadcontent.net/subsurface) and Subsurface-test for [Fedora](https://copr.fedorainfracloud.org/coprs/dirkhh/Subsurface-test).

Finally, there is a Linux AppImage (named **Subsurface-\<version\>-CICD-release.AppImage**) should run on most other x86_64 Linux distributions for which we don't have specific builds.

**Note:** Due to the asynchronous nature of our build process the artifacts for the individual platforms are added to the release one by one, whenever their build is finished. So if you can not find a particular artifact on a recent (less than 2 hours old) build, please come back later and check again.

Please report any issues with these builds in the [Subsurface user forum](https://groups.google.com/g/subsurface-divelog).

Also please note - the source tar and zip files below are created by GitHub and are essentially empty. You'll find the sources over in the [Subsurface repo](https://github.com/subsurface/subsurface)

