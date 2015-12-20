#!/bin/sh
set -e

usage () {
  echo "Usage: [VARIABLE...] $(basename $0) architecture"
  echo ""
  echo "  architecture   Target architecture. [armv7|armv7s|arm64|i386|x86_64]"
  echo ""
  echo "  VARIABLEs are:"
  echo "    SDKVERSION   Target a specific SDK version."
  echo "    PREFIX       Custom install prefix, useful for local installs."
  echo "    CHOST        Configure host, set if not deducable by ARCH."
  echo "    SDK          SDK target, set if not deducable by ARCH. [iphoneos|iphonesimulator]"
  echo ""
  echo "    CFLAGS CPPFLAGS CXXFLAGS LDFLAGS PKG_CONFIG_PATH"
  echo ""
  echo "  All additional parameters are passed to the configure script."
  exit 1
}

# Sanity checks
if [ "$#" -lt 1 ]; then
  echo "Please supply an architecture name."
  usage
fi

if [ ! -x "./configure" ] ; then
  echo "No configure script found."
  usage
fi

# Build architecture
export ARCH=$1

# Export supplied CHOST or deduce by ARCH
if [ ! -z "$CHOST" ]; then
  export CHOST
else
  case $ARCH in
    armv7 | armv7s )
      export CHOST=arm-apple-darwin*
      ;;
    arm64 )
      export CHOST=aarch64-apple-darwin*
      ;;
    i386 | x86_64 )
      export CHOST=$ARCH-apple-darwin*
      ;;
    * )
      usage
    ;;
  esac
fi

# Export supplied SDK or deduce by ARCH
if [ ! -z "$SDK" ]; then
  export SDK
else
  case $ARCH in
    armv7 | armv7s | arm64 )
      export SDK=iphoneos
      ;;
    i386 | x86_64 )
      export SDK=iphonesimulator
      ;;
    * )
      usage
    ;;
  esac
fi

# Export supplied SDKVERSION or use system default
if [ ! -z "$SDKVERSION" ]; then
  SDKNAME=$(basename $(xcrun --sdk $SDK --show-sdk-platform-path) .platform)
  export SDKVERSION
  export SDKROOT=$(xcrun --sdk $SDK --show-sdk-platform-path)"/Developer/SDKs/$SDKNAME.$SDKVERSION.sdk"
else
  export SDKVERSION=$(xcrun --sdk $SDK --show-sdk-version) # current version
  export SDKROOT=$(xcrun --sdk $SDK --show-sdk-path) # current version
fi

export PREFIX

# Binaries
export CC=$(xcrun --sdk $SDK --find gcc)
export CPP=$(xcrun --sdk $SDK --find gcc)" -E"
export CXX=$(xcrun --sdk $SDK --find g++)
export LD=$(xcrun --sdk $SDK --find ld)

# Flags
export CFLAGS="$CFLAGS -arch $ARCH -isysroot $SDKROOT -I$PREFIX/include -miphoneos-version-min=$SDKVERSION"
export CPPFLAGS="$CPPFLAGS -arch $ARCH -isysroot $SDKROOT -I$PREFIX/include -miphoneos-version-min=$SDKVERSION"
export CXXFLAGS="$CXXFLAGS -arch $ARCH -isysroot $SDKROOT -I$PREFIX/include"
export LDFLAGS="$LDFLAGS -arch $ARCH -isysroot $SDKROOT -L$PREFIX/lib"
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH":"$SDKROOT/usr/lib/pkgconfig":"$PREFIX/lib/pkgconfig"

# Remove script parameters
shift 1

# Run configure
./configure \
	--prefix="$PREFIX" \
	--host="$CHOST" \
	--enable-static \
	--disable-shared \
	$@
