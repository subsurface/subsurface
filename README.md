# Subsurface

![Build Status](https://github.com/subsurface/subsurface/workflows/Windows/badge.svg)
![Build Status](https://github.com/subsurface/subsurface/workflows/Mac/badge.svg)
![Build Status](https://github.com/subsurface/subsurface/workflows/iOS/badge.svg)
![Build Status](https://github.com/subsurface/subsurface/workflows/Android/badge.svg)

![Build Status](https://github.com/subsurface/subsurface/workflows/Linux%20Snap/badge.svg)
![Build Status](https://github.com/subsurface/subsurface/workflows/Ubuntu%2014.04%20/%20Qt%205.12%20for%20AppImage--/badge.svg)
![Build Status](https://github.com/subsurface/subsurface/workflows/Ubuntu%2018.04%20/%20Qt%205.9--/badge.svg)
![Build Status](https://github.com/subsurface/subsurface/workflows/Ubuntu%2020.10%20/%20Qt%205.14--/badge.svg)
![Build Status](https://github.com/subsurface/subsurface/workflows/openSUSE/Tumbleweed%20/%20Qt%20latest--/badge.svg)

This is the README file for Subsurface 5.0.1

Please check the `ReleaseNotes.txt` for details about new features and
changes since Subsurface 5.0 (and earlier versions).

Subsurface can be found at http://subsurface-divelog.org

Our user forum is at http://subsurface-divelog.org/user-forum/

Report bugs and issues at https://github.com/Subsurface/subsurface/issues

License: GPLv2

We frequently make new test versions of Subsurface available at
http://subsurface-divelog.org/downloads/test/ and there you can always get
the latest builds for Mac, Windows, Linux AppImage and Android (with some
caveats about installability). Additionally, those same versions are
posted to the Subsurface-daily repos on Launchpad and OBS.

These tend to contain the latest bug fixes and features, but also
occasionally the latest bugs and issues. Please understand when using them
that these are primarily intended for testing.

You can get the sources to the latest development version from the git
repository:

```
git clone https://github.com/Subsurface/subsurface.git
```

You can also fork the repository and browse the sources at the same site,
simply using https://github.com/Subsurface/subsurface

If you want the latest release (instead of the bleeding edge
development version) you can either get this via git or the release tar
ball. After cloning run the following command:

```
git checkout v5.0.1  (or whatever the last release is)
```

or download a tarball from http://subsurface-divelog.org/downloads/Subsurface-5.0.1.tgz

Detailed build instructions can be found in the INSTALL file.

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
this is also available as Documentation/user-manual.html. The
documentation for the latest release is also available on-line
http://subsurface-divelog.org/documentation/

## Contributing

There is a mailing list for developers: subsurface@subsurface-divelog.org
Go to http://lists.subsurface-divelog.org/cgi-bin/mailman/listinfo/subsurface
to subscribe.

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
