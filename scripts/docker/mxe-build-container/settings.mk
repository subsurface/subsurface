# This is a template of configuration file for MXE. See
# index.html for more extensive documentations.

# This variable controls the number of compilation processes
# within one package ("intra-package parallelism").
JOBS := 8

# This variable controls the targets that will build.
MXE_TARGETS :=  x86_64-w64-mingw32.shared

# The three lines below makes `make` build these "local packages" instead of all packages.
LOCAL_PKG_LIST := nsis \
                  curl \
                  libxml2 \
                  libxslt \
                  libzip \
                  libusb1 \
                  hidapi \
                  libgit2 \
                  libftdi1 \
                  mdbtools \
                  qtbase \
                  qtconnectivity \
                  qtdeclarative \
                  qtimageformats \
                  qtlocation \
                  qtmultimedia \
                  qtwebkit \
                  qtquickcontrols \
                  qtquickcontrols2 \
                  qtcharts \
                  qtsvg \
                  qttools \
                  qttranslations \
                  zstd
.DEFAULT local-pkg-list:
local-pkg-list: $(LOCAL_PKG_LIST)
