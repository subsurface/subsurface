# Subsurface

[![Windows](https://github.com/subsurface/subsurface/actions/workflows/windows.yml/badge.svg)](https://github.com/subsurface/subsurface/actions/workflows/windows.yml)
[![Mac](https://github.com/subsurface/subsurface/actions/workflows/mac.yml/badge.svg)](https://github.com/subsurface/subsurface/actions/workflows/mac.yml)
[![iOS](https://github.com/subsurface/subsurface/actions/workflows/ios.yml/badge.svg)](https://github.com/subsurface/subsurface/actions/workflows/ios.yml)
[![Android](https://github.com/subsurface/subsurface/actions/workflows/android.yml/badge.svg)](https://github.com/subsurface/subsurface/actions/workflows/android.yml)

[![Snap](https://github.com/subsurface/subsurface/actions/workflows/linux-snap.yml/badge.svg)](https://github.com/subsurface/subsurface/actions/workflows/linux-snap.yml)
[![Ubuntu 20.04 / Qt 5.15-- for AppImage](https://github.com/subsurface/subsurface/actions/workflows/linux-ubuntu-20.04-qt5-appimage.yml/badge.svg)](https://github.com/subsurface/subsurface/actions/workflows/linux-ubuntu-20.04-qt5-appimage.yml)
[![Ubuntu 24.04 / Qt 5.15--](https://github.com/subsurface/subsurface/actions/workflows/linux-ubuntu-24.04-qt5.yml/badge.svg)](https://github.com/subsurface/subsurface/actions/workflows/linux-ubuntu-24.04-qt5.yml)
[![Fedora 35 / Qt 6--](https://github.com/subsurface/subsurface/actions/workflows/linux-fedora-35-qt6.yml/badge.svg)](https://github.com/subsurface/subsurface/actions/workflows/linux-fedora-35-qt6.yml)
[![Debian Bookworm / Qt 5.15--](https://github.com/subsurface/subsurface/actions/workflows/linux-debian-bookworm-5.15.yml/badge.svg)](https://github.com/subsurface/subsurface/actions/workflows/linux-debian-bookworm-5.15.yml)

[![Coverity Scan Results](https://scan.coverity.com/projects/14405/badge.svg)](https://scan.coverity.com/projects/subsurface-divelog-subsurface)

Subsurface can be found at http://subsurface-divelog.org

Our user forum is at http://subsurface-divelog.org/user-forum/

Report bugs and issues at https://github.com/Subsurface/subsurface/issues

License: GPLv2

We are releasing 'nightly' builds of Subsurface that are built from the latest version of the code. Versions of this build for Windows, macOS, Android (requiring sideloading), and a Linux AppImage can be downloaded from the [Latest Dev Release](https://www.subsurface-divelog.org/latest-release/) page on [our website](https://www.subsurface-divelog.org/). Alternatively, they can be downloaded [directly from GitHub](https://github.com/subsurface/nightly-builds/releases). Additionally, those same versions are
posted to the Subsurface-daily repos on Ubuntu Launchpad, Fedora COPR, and
OpenSUSE OBS, and released to [Snapcraft](https://snapcraft.io/subsurface) into the 'edge' channel of subsurface.

You can get the sources to the latest development version from the git
repository:

```
git clone https://github.com/Subsurface/subsurface.git
```

You can also fork the repository and browse the sources at the same site,
simply using https://github.com/Subsurface/subsurface

Additionally, artifacts for Windows, macOS, Android, Linux AppImage, and iOS (simulator build) are generated for all open pull requests and linked in pull request comments. Use these if you want to test the changes in a specific pull request and provide feedback before it has been merged.

If you want a more stable version that is a little bit more tested you can get this from the [Curent Release](https://www.subsurface-divelog.org/current-release/) page on [our website](https://www.subsurface-divelog.org/).

Detailed build instructions can be found in the [INSTALL.md](/INSTALL.md) file.

## System Requirements

On desktop, the integrated Googlemaps feature of Subsurface requires a GPU
driver that has support for at least OpenGL 2.1. If your driver does not
support that, you may have to run Subsurface in software renderer mode.

Subsurface will automatically attempt to detect this scenario, but in case
it doesn't you may have to enable the software renderer manually with
the following:
1) Learn how to set persistent environment variables on your OS
2) Set the environment variable 'QT_QUICK_BACKEND' with the value of 'software'

## Basic Usage

Install and start from the desktop, or you can run it locally from the
build directory:

On Linux:

```
$ ./subsurface
```

On Mac:

```
$ open Subsurface.app
```

Native builds on Windows are not really supported (the official Windows
installers are cross-built on Linux).

You can give a data file as command line argument, or (once you have
set this up in the Preferences) Subsurface picks a default file for
you when started from the desktop or without an argument.

If you have a dive computer supported by libdivecomputer, you can just
select "Import from Divecomputer" from the "Import" menu, select which
dive computer you have (and where it is connected if you need to - note
that there's a special selection for Bluetooth dive computers), and click
on "Download".

The latest list of supported dive computers can be found in the file
SupportedDivecomputers.txt.

Much more detailed end user instructions can be found from inside
Subsurface by selecting Help (typically F1). When building from source
this is also available as Documentation/user-manual.txt. The
documentation for the latest release is also available on-line
http://subsurface-divelog.org/documentation/

## Contributing

There is a user forum for questions, bug reports, and feature requests:
https://groups.google.com/g/subsurface-divelog

If you want to contribute code, please open a pull request with signed-off
commits at https://github.com/Subsurface/subsurface/pulls
(alternatively, you can also send your patches as emails to the developer
mailing list).

Either way, if you don't sign off your patches, we will not accept them.
This means adding a line that says "Signed-off-by: Name <email>" at the
end of each commit, indicating that you wrote the code and have the right
to pass it on as an open source patch under the GPLv2 license.

See: http://developercertificate.org/

Also, please write good git commit messages.  A good commit message
looks like this:

```
Header line: explain the commit in one line (use the imperative)

Body of commit message is a few lines of text, explaining things
in more detail, possibly giving some background about the issue
being fixed, etc etc.

The body of the commit message can be several paragraphs, and
please do proper word-wrap and keep columns shorter than about
74 characters or so. That way "git log" will show things
nicely even when it's indented.

Make sure you explain your solution and why you're doing what you're
doing, as opposed to describing what you're doing. Reviewers and your
future self can read the patch, but might not understand why a
particular solution was implemented.

Reported-by: whoever-reported-it
Signed-off-by: Your Name <you@example.com>
```

where that header line really should be meaningful, and really should be
just one line.  That header line is what is shown by tools like gitk and
shortlog, and should summarize the change in one readable line of text,
independently of the longer explanation. Please use verbs in the
imperative in the commit message, as in "Fix bug that...", "Add
file/feature ...", or "Make Subsurface..."

## A bit of Subsurface history

In fall of 2011, when a forced lull in kernel development gave him an
opportunity to start on a new endeavor, Linus Torvalds decided to tackle
his frustration with the lack of decent divelog software on Linux.

Subsurface is the result of the work of him and a team of developers since
then. It now supports Linux, Windows and MacOS and allows data import from
a large number of dive computers and several existing divelog programs. It
provides advanced visualization of the key information provided by a
modern dive computer and allows the user to track a wide variety of data
about their diving.

In fall of 2012 Dirk Hohndel took over as maintainer of Subsurface.
