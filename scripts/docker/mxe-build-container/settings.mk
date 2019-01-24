# This is a template of configuration file for MXE. See
# index.html for more extensive documentations.

# This variable controls the number of compilation processes
# within one package ("intra-package parallelism").
JOBS := 6

# This variable controls the targets that will build.
MXE_TARGETS :=  i686-w64-mingw32.shared

# The three lines below makes `make` build these "local packages" instead of all packages.
LOCAL_PKG_LIST := qtbase qtconnectivity qtdeclarative qtimageformats qtlocation qtmultimedia qtquickcontrols qtquickcontrols2 qtscript qtsvg qttools qttranslations qtwebview qtwebkit libxml2 libxslt libusb1 libgit2 nsis curl libzip libftdi1
.DEFAULT local-pkg-list:
local-pkg-list: $(LOCAL_PKG_LIST)

