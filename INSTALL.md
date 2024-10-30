# Building Subsurface from Source

Subsurface uses quite a few open source libraries and frameworks to do its
job. The most important ones include libdivecomputer, Qt, libxml2, libxslt,
libsqlite3, libzip, and libgit2.

Below are instructions for building Subsurface
- on some popular Linux distributions,
- MacOSX,
- Windows (cross-building)
- Android (cross-building)
- iOS (cross-building)


## Getting Subsurface source

You can get the sources to the latest development version from our git
repository:

```
git clone http://github.com/Subsurface/subsurface.git
cd subsurface
git submodule init # this will give you our flavor of libdivecomputer
```

You keep it updated by doing:

```
git checkout master
git pull -r
git submodule update
```


### Our flavor of libdivecomputer

Subsurface requires its own flavor of libdivecomputer which is inclduded
above as git submodule

The branches won't have a pretty history and will include ugly merges,
but they should always allow a fast forward pull that tracks what we
believe developers should build against. All our patches are contained
in the `Subsurface-DS9` branch.

This should allow distros to see which patches we have applied on top of
upstream. They will receive force pushes as we rebase to newer versions of
upstream so they are not ideal for ongoing development (but they are of
course easy to use for distributions as they always build "from scratch",
anyway).

The rationale for this is that we have no intention of forking the
project. We simply are adding a few patches on top of their latest
version and want to do so in a manner that is both easy for our
developers who try to keep them updated frequently, and anyone packaging
Subsurface or trying to understand what we have done relative to their
respective upstreams.


### Getting Qt5

We use Qt5 in order to only maintain one UI across platforms.

Qt5.9.1 is the oldest version supported if ONLY building Subsurface
Qt5.12 is the oldest version supported if also building Subsurface-mobile

Most Linux distributions include a new enough version of Qt (and if you are on
a distro that still ships with an older Qt, likely your C compiler is also not
new enough to build Subsurface).

If you need Qt (likely on macOS) or want a newer version than provided by your
Linux distro, you can install a separate version that Subsurface will use.
As of Qt5.15 it has become a lot harder to download and install Qt - you
now need a Qt account and the installer tool has a new space age look and
significantly reduced flexibility.

As of this writing, there is thankfully a thirdparty offline installer still
available:

```
pip3 install aqtinstall
aqt install -O <Qt Location> 5.15.2 mac desktop
```

(or whatever version / OS you need). This installer is surprisingly fast
and seems well maintained - note that we don't use this for Windows as
that is completely built from source using MXE.

In order to use this Qt installation, simply add it to your PATH:

```
PATH=<Qt Location>/<version>/<type>/bin:$PATH
```

QtWebKit is needed, if you want to print, but no longer part of Qt5,
so you need to download it and compile. In case you just want to test
without print possibility omit this step.

```
git clone -b 5.212 https://github.com/qt/qtwebkit
mkdir -p qtwebkit/WebKitBuild/Release
cd qtwebkit/WebKitBuild/Release
cmake -DPORT=Qt -DCMAKE_BUILD_TYPE=Release -DQt5_DIR=/<Qt Location>/<version>/<type>/lib/cmake/Qt5 ../..
make install
```


### Other third party library dependencies

In order for our cloud storage to be fully functional you need
libgit2 0.26 or newer.


### cmake build system

Our main build system is based on cmake. But qmake is needed
for the googlemaps plugin and the iOS build.

Download from https://cmake.org/download and follow the instructions
to install it or alternatively follow the instruction specific to a
distribution (see build instructions).



## Build options for Subsurface

The following options are recognised when passed to cmake:

`-DCMAKE_BUILD_TYPE=Release` create a release build
`-DCMAKE_BUILD_TYPE=Debug` create a debug build

The Makefile that was created using cmake can be forced into a much more
verbose mode by calling

```
make VERBOSE=1
```

Many more variables are supported, the easiest way to interact with them is
to call

```
ccmake .
```

in your build directory.


### Building the development version of Subsurface under Linux

On Fedora you need

```
sudo dnf install autoconf automake bluez-libs-devel cmake gcc-c++ git \
    libcurl-devel libsqlite3x-devel libssh2-devel libtool libudev-devel \
    libusbx-devel libxml2-devel libxslt-devel make \
    qt5-qtbase-devel qt5-qtconnectivity-devel qt5-qtdeclarative-devel \
    qt5-qtlocation-devel qt5-qtscript-devel qt5-qtsvg-devel \
    qt5-qttools-devel qt5-qtwebkit-devel redhat-rpm-config \
    bluez-libs-devel libgit2-devel libzip-devel libmtp-devel libraw-devel
```


Package names are sadly different on OpenSUSE

```
sudo zypper install git gcc-c++ make autoconf automake libtool cmake libzip-devel \
    libxml2-devel libxslt-devel sqlite3-devel libusb-1_0-devel \
    libqt5-linguist-devel libqt5-qttools-devel libQt5WebKitWidgets-devel \
    libqt5-qtbase-devel libQt5WebKit5-devel libqt5-qtsvg-devel \
    libqt5-qtscript-devel libqt5-qtdeclarative-devel \
    libqt5-qtconnectivity-devel libqt5-qtlocation-devel libcurl-devel \
    bluez-devel libgit2-devel libmtp-devel libraw-devel
```
On Debian Bookworm this seems to work
```
sudo apt install \
    autoconf automake cmake g++ git libbluetooth-dev libcrypto++-dev \
    libcurl4-openssl-dev libgit2-dev libqt5qml5 libqt5quick5 libqt5svg5-dev \
    libqt5webkit5-dev libsqlite3-dev libssh2-1-dev libssl-dev libtool \
    libusb-1.0-0-dev libxml2-dev libxslt1-dev libzip-dev make pkg-config \
    qml-module-qtlocation qml-module-qtpositioning qml-module-qtquick2 \
    qt5-qmake qtchooser qtconnectivity5-dev qtdeclarative5-dev \
    qtdeclarative5-private-dev qtlocation5-dev qtpositioning5-dev \
    qtscript5-dev qttools5-dev qttools5-dev-tools libmtp-dev libraw-dev
```

In order to build and run mobile-on-desktop, you also need

```
sudo apt install \
    qtquickcontrols2-5-dev qml-module-qtquick-window2 qml-module-qtquick-dialogs \
    qml-module-qtquick-layouts qml-module-qtquick-controls2 qml-module-qtquick-templates2 \
    qml-module-qtgraphicaleffects qml-module-qtqml-models2 qml-module-qtquick-controls
```


Package names for Ubuntu 21.04

```
sudo apt install \
    autoconf automake cmake g++ git libbluetooth-dev libcrypto++-dev \
    libcurl4-gnutls-dev libgit2-dev libqt5qml5 libqt5quick5 libqt5svg5-dev \
    libqt5webkit5-dev libsqlite3-dev libssh2-1-dev libssl-dev libtool \
    libusb-1.0-0-dev libxml2-dev libxslt1-dev libzip-dev make pkg-config \
    qml-module-qtlocation qml-module-qtpositioning qml-module-qtquick2 \
    qt5-qmake qtchooser qtconnectivity5-dev qtdeclarative5-dev \
    qtdeclarative5-private-dev qtlocation5-dev qtpositioning5-dev \
    qtscript5-dev qttools5-dev qttools5-dev-tools libmtp-dev libraw-dev
```

In order to build and run mobile-on-desktop, you also need

```
sudo apt install \
    qtquickcontrols2-5-dev qml-module-qtquick-window2 qml-module-qtquick-dialogs \
    qml-module-qtquick-layouts qml-module-qtquick-controls2 qml-module-qtquick-templates2 \
    qml-module-qtgraphicaleffects qml-module-qtqml-models2 qml-module-qtquick-controls
```


On Raspberry Pi (Raspian Buster and Ubuntu Mate 20.04.1) this seems to work

```
sudo apt install \
    autoconf automake cmake g++ git libbluetooth-dev libcrypto++-dev \
    libcurl4-gnutls-dev libgit2-dev libqt5qml5 libqt5quick5 libqt5svg5-dev \
    libqt5webkit5-dev libsqlite3-dev libssh2-1-dev libssl-dev libtool \
    libusb-1.0-0-dev libxml2-dev libxslt1-dev libzip-dev make pkg-config \
    qml-module-qtlocation qml-module-qtpositioning qml-module-qtquick2 \
    qt5-qmake qtchooser qtconnectivity5-dev qtdeclarative5-dev \
    qtdeclarative5-private-dev qtlocation5-dev qtpositioning5-dev \
    qtscript5-dev qttools5-dev qttools5-dev-tools libmtp-dev libraw-dev
```

In order to build and run mobile-on-desktop, you also need

```
sudo apt install \
    qtquickcontrols2-5-dev qml-module-qtquick-window2 qml-module-qtquick-dialogs \
    qml-module-qtquick-layouts qml-module-qtquick-controls2 qml-module-qtquick-templates2 \
    qml-module-qtgraphicaleffects qml-module-qtqml-models2 qml-module-qtquick-controls
```


Note that on Ubuntu Mate on the Raspberry Pi, you may need to configure
some swap space in order for the build to complete successfully. There is no
swap space configured by default. See the dphys-swapfile package.

On Raspberry Pi OS with Desktop (64-bit) Released April 4th, 2022, this seems
to work

```
sudo apt install \
    autoconf automake cmake g++ git libbluetooth-dev libcrypto++-dev \
    libcurl4-gnutls-dev libgit2-dev libqt5qml5 libqt5quick5 libqt5svg5-dev \
    libqt5webkit5-dev libsqlite3-dev libssh2-1-dev libssl-dev libtool \
    libusb-1.0-0-dev libxml2-dev libxslt1-dev libzip-dev make pkg-config \
    qml-module-qtlocation qml-module-qtpositioning qml-module-qtquick2 \
    qt5-qmake qtchooser qtconnectivity5-dev qtdeclarative5-dev \
    qtdeclarative5-private-dev qtlocation5-dev qtpositioning5-dev \
    qtscript5-dev qttools5-dev qttools5-dev-tools libmtp-dev libraw-dev
```

Note that you'll need to increase the swap space as the default of 100MB
doesn't seem to be enough.  1024MB worked on a 3B+.

If maps aren't working, copy the googlemaps plugin
from `<build_dir>/subsurface/googlemaps/build/libqtgeoservices_googlemaps.so`
to `/usr/lib/aarch64-linux-gnu/qt5/plugins/geoservices/`.

If Subsurface can't seem to see your dive computer on `/dev/ttyUSB0`, even after
adjusting your account's group settings (see note below about usermod), it
might be that the FTDI driver doesn't recognize the VendorID/ProductID of your
computer. Follow the instructions here:

https://www.ftdichip.com/Support/Documents/TechnicalNotes/TN_101_Customising_FTDI_VID_PID_In_Linux(FT_000081).pdf

If you're unsure of the VID/PID of your device, plug your dive computer in to
your host and run `dmesg`. That should show the codes that are needed to
follow TN_101.

On PCLinuxOS you appear to need the following packages

```
su -c "apt-get install -y autoconf automake cmake gcc-c++ git libtool \
    lib64bluez-devel lib64qt5bluetooth-devel lib64qt5concurrent-devel \
    lib64qt5help-devel lib64qt5location-devel lib64qt5quicktest-devel \
    lib64qt5quickwidgets-devel lib64qt5script-devel lib64qt5svg-devel \
    lib64qt5test-devel lib64qt5webkitwidgets-devel lib64qt5xml-devel \
    lib64ssh2-devel lib64usb1.0-devel lib64zip-devel qttools5 qttranslations5"
```

In order to build Subsurface, use the supplied build script. This should
work on most systems that have all the prerequisite packages installed.

You should have Subsurface sources checked out in a sane place, something
like this:

```
mkdir -p ~/src
cd ~/src
git clone https://github.com/Subsurface/subsurface.git
./subsurface/scripts/build.sh # <- this step will take quite a while as it
                              #    compiles a handful of libraries before
                              #    building Subsurface
```

Now you can run Subsurface like this:

```
cd ~/src/subsurface/build
./subsurface
```


Note: on many Linux versions (for example on Kubuntu 15.04) the user must
belong to the `dialout` group.

You may need to run something like

```
sudo usermod -a -G dialout $USER
```

with your correct username and log out and log in again for that to take
effect.

If you get errors like:

```
./subsurface: error while loading shared libraries: libGrantlee_Templates.so.5: cannot open shared object file: No such file or directory
```

You can run the following command:

```
sudo ldconfig ~/src/install-root/lib
```


### Building Subsurface under MacOSX

While it is possible to build all required components completely from source,
at this point the preferred way to build Subsurface is to set up the build
infrastructure via Homebrew and then build the dependencies from source.

0. You need to have XCode installed. The first time (and possibly after updating OSX)

```
xcode-select --install
```

1. install Homebrew (see https://brew.sh) and then the required build infrastructure:

```
brew install autoconf automake libtool pkg-config gettext confuse
```

2. install Qt

download the macOS installer from https://download.qt.io/official_releases/online_installers
and use it to install the desired Qt version. At this point the latest Qt5 version is still
preferred over Qt6.

If you plan to deploy your build to an Apple Silicon Mac, you may have better results with
Bluetooth connections if you install Qt5.15.13. If Qt5.15.13 is not available via the
installer, you can download from https://download.qt.io/official_releases/qt/5.15/5.15.13
and build using the usual configure, make, and make install. Qt is also available as Homebrew package


3. now build Subsurface

```
cd ~/src; bash subsurface/scripts/build.sh -build-deps
```

if you are building against Qt6 (still experimental) you can create a universal binary with

```
cd ~/src; bash subsurface/scripts/build.sh -build-with-qt6 -build-deps -fat-build
```

4. Sign the package
```
codesign --options runtime --keychain $HOME/Library/Keychains/login.keychain --sign - --deep --force --entitlements subsurface/build/entitlements-mac-dev.plist subsurface/build/Subsurface.app
```

After the above is done, Subsurface.app will be available in the
subsurface/build directory. You can run Subsurface with the command

A. `open subsurface/build/Subsurface.app`
   this will however not show diagnostic output

B. `subsurface/build/Subsurface.app/Contents/MacOS/Subsurface`
   the [Tab] key is your friend :-)

Debugging can be done with either Xcode or QtCreator.

To install the app for all users, move subsurface/build/Subsurface.app to /Applications.


### Cross-building Subsurface on MacOSX for iOS

0. build SubSurface under MacOSX and iOS

1. `cd <repo>/..; bash <repo>/scripts/build.sh -build-deps -both`
note: this is mainly done to ensure all external dependencies are downloaded and set
      to the correct versions

2. follow [these instructions](packaging/ios/README.md)



### Cross-building Subsurface on Linux for Windows

Subsurface for Windows builds on linux by using the [MXE (M cross environment)](https://github.com/mxe/mxe). The easiest way to do this is to use a Docker container with a pre-built MXE for Subsurface by following [these instructions](packaging/windows/README.md).


### Building Subsurface on Windows

This is NOT RECOMMENDED. To the best of our knowledge there is one single
person who regularly does this. The Subsurface team does not provide support
for Windows binary build from sources natively under Windows...

The lack of a working package management system for Windows makes it
really painful to build Subsurface natively under Windows,
so we don't support that at all.

But if you want to build Subsurface on a Windows system, the docker based [cross-build for Windows](packaging/windows/README.md) works just fine in WSL2 on Windows.


### Cross-building Subsurface on Linux for Android

Follow [these instructions](packaging/android/README.md).
