#!/bin/bash
#
# this simply automates the steps to create a DMG we can ship
#
# run this from the top subsurface directory

# make it verbose in a GitHub Action
[[ -n $RUNNER_OS ]] && set -x

# find the directory above the sources - typically ~/src
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd ../../.. && pwd )

DMGCREATE=create-dmg

# same git version magic as in the Makefile
# for the naming of volume and dmg we want the 3 digits of the full version number
if [ -z "${CANONICALVERSION+X}" ] ; then
        CANONICALVERSION=$(cd ${DIR}/subsurface; ./scripts/get-version.sh)
fi

# first build and install Subsurface and then clean up the staging area
# make sure we didn't lose the minimum OS version
rm -rf ./Subsurface.app

# Apple tells us NOT to try to specify a specific SDK anymore and instead let its tools
# do "the right thing" - especially don't set a sysroot (which way back when was required for this to work)
# unforunately that means we need to somehow hard-code a deployment target, hoping the local tools
# know how to build for that. Which seems... odd
ARCH=$(uname -m) # crazy, I know, but $(arch) results in the incorrect 'i386' on an x86_64 Mac
OLDER_MAC_CMAKE="-DCMAKE_OSX_DEPLOYMENT_TARGET=${BASESDK} -DCMAKE_OSX_ARCHITECTURES="$ARCH" -DCMAKE_BUILD_TYPE={$DEBUGRELEASE} -DCMAKE_INSTALL_PREFIX=${INSTALL_ROOT} -DCMAKE_POLICY_VERSION_MINIMUM=3.16"
export PKG_CONFIG_PATH=${DIR}/install-root/lib/pkgconfig:$PKG_CONFIG_PATH

cmake $OLDER_MAC_CMAKE \
	-DLIBGIT2_FROM_PKGCONFIG=ON \
	-DLIBGIT2_DYNAMIC=ON \
	-DFTDISUPPORT=ON \
	.

LIBRARY_PATH=${DIR}/install-root/lib make

LIBRARY_PATH=${DIR}/install-root/lib make install

# next, let's make sure we have single architecture libraries and frameworks, not fat binaries
find Subsurface.app/Contents/Frameworks -type f \( -name "Qt*" -o -name "*.dylib" \) | while read -r lib; do
    if [[ $(lipo "$lib" -archs)  != "$ARCH" ]] 2>/dev/null; then
        lipo "$lib" -thin "$ARCH" -output "$lib.tmp" && mv "$lib.tmp" "$lib"
    fi
done

# Remove unnecessary style modules (keep only macOS for a macOS app)
APP_BUNDLE="./Subsurface.app"
QML_CONTROLS="$APP_BUNDLE/Contents/Resources/qml/QtQuick/Controls"
for style in Basic Fusion Material Universal Imagine FluentWinUI3 iOS; do
	rm -rf "${QML_CONTROLS}/${style}"
done

# also remove debug symbols to save more space
rm -rf "$QML_CONTROLS/libqtquickcontrols2plugin.dylib.dSYM"

# now adjust a few references that macdeployqt appears to miss
EXECUTABLE=Subsurface.app/Contents/MacOS/Subsurface
OLD=$(otool -L ${EXECUTABLE} | grep libgit2 | cut -d\  -f1 | tr -d "\t")
if [[ ! -z ${OLD} && ! -f Subsurface.app/Contents/Frameworks/$(basename ${OLD}) ]] ; then
	# copy the library into the bundle and make sure its id and the reference to it are correct
	cp ${DIR}/install-root/lib/$(basename ${OLD}) Subsurface.app/Contents/Frameworks
	SONAME=$(basename $OLD)
	install_name_tool -change ${OLD} @executable_path/../Frameworks/${SONAME} ${EXECUTABLE}
	install_name_tool -id @executable_path/../Frameworks/${SONAME} Subsurface.app/Contents/Frameworks/${SONAME}
	CURLLIB=$(otool -L Subsurface.app/Contents/Frameworks/${SONAME} | grep libcurl | cut -d\  -f1 | tr -d "\t")
	install_name_tool -change ${CURLLIB} @executable_path/../Frameworks/$(basename ${CURLLIB}) Subsurface.app/Contents/Frameworks/${SONAME}
fi

# ensure libpng and libjpeg inside the bundle are referenced in QtWebKit libraries
QTWEBKIT=Subsurface.app/Contents/Frameworks/QtWebKit.framework/QtWebKit
if [ -f $QTWEBKIT ] ; then
    for i in libjpeg.8.dylib libpng16.16.dylib; do
	OLD=$(otool -L ${QTWEBKIT} | grep $i | cut -d\  -f1 | tr -d "\t")
        if [[ ! -z ${OLD} ]] ; then
                # copy the library into the bundle and make sure its id and the reference to it are correct
		if [[ ! -f Subsurface.app/Contents/Frameworks/$(basename ${OLD}) ]] ; then
			cp ${OLD} Subsurface.app/Contents/Frameworks
		fi
                SONAME=$(basename $OLD)
                install_name_tool -change ${OLD} @executable_path/../Frameworks/${SONAME} ${QTWEBKIT}
                install_name_tool -id @executable_path/../Frameworks/${SONAME} Subsurface.app/Contents/Frameworks/${SONAME}
        fi
    done
fi

# next, replace @rpath references with @executable_path references in Subsurface
RPATH=$(otool -L ${EXECUTABLE} | grep rpath  | cut -d\  -f1 | tr -d "\t" | cut -b 8- )
for i in ${RPATH}; do
	install_name_tool -change @rpath/$i @executable_path/../Frameworks/$i ${EXECUTABLE}
done

if [ "$1" = "-nodmg" ] ; then
	exit 0
elif [ "$1" = "-sign" ] ; then
	SIGN=1
else
	SIGN=0
fi

echo "=== Validating macOS App Bundle: $APP_BUNDLE ==="
echo ""
EXIT_CODE=0

# 1. Check bundle structure
echo "1. Checking bundle structure..."
if [ ! -d "$APP_BUNDLE/Contents/MacOS" ]; then
    echo "❌ Missing Contents/MacOS directory"
    EXIT_CODE=1
else
    echo "✅ Bundle structure looks good"
fi

# 2. Check executable exists and is executable
echo ""
echo "2. Checking main executable..."
EXECUTABLE=$(find "$APP_BUNDLE/Contents/MacOS" -type f -perm +111 | head -1)
if [ -z "$EXECUTABLE" ]; then
    echo "❌ No executable found"
    EXIT_CODE=1
else
    echo "✅ Found executable: $EXECUTABLE"
fi

# 3. Check code signature
echo ""
echo "3. Checking code signature..."
if codesign -vv "$APP_BUNDLE" 2>&1 | grep -q "valid on disk"; then
    echo "✅ Code signature is valid"
else
    echo "⚠️  Code signature check failed (may be expected for ad-hoc signing)"
fi

# 4. Check all library dependencies can be resolved
echo ""
echo "4. Checking library dependencies..."
MISSING_LIBS=0
BAD_LIBS=""

for lib in $(find "$APP_BUNDLE" -type f \( -name "*.dylib" -o -perm +111 \) 2>/dev/null); do
    # Skip script files
    file "$lib" 2>/dev/null | grep -q "Mach-O" || continue

    # Get library basename for self-reference check
    LIBNAME=$(basename "$lib")

    # Get all dependencies, excluding the first line (install name) and skipping column 1
    # Format is: \t<path> (compatibility...)
    DEPS=$(otool -L "$lib" 2>/dev/null | tail -n +2 | awk '{print $1}')

    for dep in $DEPS; do
        # Skip if it's a self-reference (library depends on itself)
        DEPNAME=$(basename "$dep")
        [[ "$DEPNAME" == "$LIBNAME" ]] && continue

        # Skip if it's a relative path (@rpath, @loader_path, @executable_path)
        [[ "$dep" == @* ]] && continue

        # Skip if it's within the bundle itself
        [[ "$dep" == *"$APP_BUNDLE"* ]] && continue

        # Skip if it's a system framework or library
        [[ "$dep" == /System/* ]] && continue
        [[ "$dep" == /usr/lib/* ]] && continue
        [[ "$dep" == /Library/Frameworks/* ]] && continue

        # If we get here, it's a problematic absolute path
        echo "   ⚠️  $LIBNAME has external dependency: $dep"
        BAD_LIBS="$BAD_LIBS\n  $LIBNAME -> $dep"
        MISSING_LIBS=1
    done
done

if [ $MISSING_LIBS -eq 0 ]; then
    echo "✅ All libraries use bundle-relative or system paths"
else
    echo "❌ Found libraries with problematic absolute path dependencies:"
    echo -e "$BAD_LIBS"
    EXIT_CODE=1
fi

# 5. Check critical Qt frameworks are present
echo ""
echo "5. Checking Qt frameworks..."
REQUIRED_FW=("QtCore" "QtGui" "QtWidgets")
for fw in "${REQUIRED_FW[@]}"; do
    if [ -d "$APP_BUNDLE/Contents/Frameworks/$fw.framework" ]; then
        echo "✅ $fw.framework present"
    else
        echo "❌ Missing $fw.framework"
        EXIT_CODE=1
    fi
done

# 6. Specifically check libqlitehtml RPATH
echo ""
echo "6. Checking libqlitehtml RPATH..."
LIBQLITE=$(find "$APP_BUNDLE" -name "libqlitehtml*.dylib" 2>/dev/null | head -1)
if [ -n "$LIBQLITE" ]; then
    echo "   Found: $(basename $LIBQLITE)"

    # Get all RPATHs
    RPATHS=$(otool -l "$LIBQLITE" | grep -A 2 LC_RPATH | grep path | awk '{print $2}')

    if [ -z "$RPATHS" ]; then
        echo "   ℹ️  No RPATH set (may use default)"
    else
        # Check if there are any problematic absolute RPATHs
        BAD_RPATH=0
        for rpath in $RPATHS; do
            # Relative RPATHs are good
            if [[ "$rpath" == @* ]]; then
                echo "   ✅ Good RPATH: $rpath"
            # System paths are OK
            elif [[ "$rpath" == /System/* ]] || [[ "$rpath" == /usr/lib/* ]]; then
                echo "   ✅ System RPATH: $rpath"
            # Absolute paths to user directories are bad
            elif [[ "$rpath" == /Users/* ]] || [[ "$rpath" == /home/* ]] || [[ "$rpath" == /opt/* ]]; then
                echo "   ❌ Problematic RPATH: $rpath"
                BAD_RPATH=1
            else
                echo "   ⚠️  Unknown RPATH: $rpath"
            fi
        done

        if [ $BAD_RPATH -eq 1 ]; then
            echo ""
            echo "   🔧 Auto-fixing problematic RPATHs..."

            # Delete all absolute RPATHs (anything not starting with @)
            for rpath in $RPATHS; do
                if [[ ! "$rpath" =~ ^@ ]]; then
                    echo "   Removing absolute RPATH: $rpath"
                    install_name_tool -delete_rpath "$rpath" "$LIBQLITE" 2>/dev/null || true
                fi
            done

            # Add the correct relative RPATH if not already present
            if ! otool -l "$LIBQLITE" | grep -A 2 LC_RPATH | grep -q '@loader_path/..'; then
                echo "   Adding correct RPATH: @loader_path/.."
                install_name_tool -add_rpath '@loader_path/..' "$LIBQLITE"
            fi

            echo "   ✅ RPATH fixed!"

            # Verify the fix worked
            echo "   Verifying fix..."
            NEW_RPATHS=$(otool -l "$LIBQLITE" | grep -A 2 LC_RPATH | grep path | awk '{print $2}')
            BAD_STILL=0
            for rpath in $NEW_RPATHS; do
                if [[ "$rpath" == /Users/* ]] || [[ "$rpath" == /home/* ]] || [[ "$rpath" == /opt/* ]]; then
                    echo "   ❌ Still has bad RPATH: $rpath"
                    BAD_STILL=1
                fi
            done

            if [ $BAD_STILL -eq 0 ]; then
                echo "   ✅ All RPATHs are now correct"
            else
                echo "   ❌ Failed to fix all RPATHs"
                EXIT_CODE=1
            fi
        fi
    fi
else
    echo "⚠️  libqlitehtml not found (may not be included)"
fi

# 7. Check bundle size
echo ""
echo "7. Bundle size:"
du -sh "$APP_BUNDLE"

# 8. Check for unnecessary QML bloat
echo ""
echo "8. Checking for QML bloat..."
QML_CONTROLS="$APP_BUNDLE/Contents/Resources/qml/QtQuick/Controls"
if [ -d "$QML_CONTROLS" ]; then
    CONTROLS_SIZE=$(du -sh "$QML_CONTROLS" | awk '{print $1}')
    echo "   QtQuick/Controls size: $CONTROLS_SIZE"

    # Check for unnecessary styles
    BLOAT_FOUND=0
    for style in Basic Fusion Material Universal Imagine FluentWinUI3 iOS; do
        if [ -d "$QML_CONTROLS/$style" ]; then
            echo "   ⚠️  Unnecessary style found: $style"
            BLOAT_FOUND=1
        fi
    done

    if [ $BLOAT_FOUND -eq 0 ]; then
        echo "   ✅ No unnecessary QML styles found"
    else
        echo "   ⚠️  Consider removing unnecessary QML styles to reduce bundle size"
    fi

    # Check for macOS style
    if [ -d "$QML_CONTROLS/macOS" ]; then
        echo "   ✅ macOS style present"
    fi
else
    echo "   ℹ️  QtQuick/Controls not found"
fi

echo ""
if [ "$EXIT_CODE" -eq 0 ]; then
    echo "=== ✅ Bundle validation PASSED ==="
else
    echo "=== ❌ Bundle validation FAILED ==="
	exit 1
fi

# copy things into staging so we can create a nice DMG
rm -rf ./staging
mkdir ./staging
cp -a ./Subsurface.app ./staging

if [ "$SIGN" = "1" ] ; then # that's for local builds for Dirk... oops
	sh ${DIR}/subsurface/packaging/macosx/sign
else
	codesign --force --deep --sign - staging/Subsurface.app
fi

echo "Cleaning up any mounted volumes..."
hdiutil info | grep "/Volumes/Subsurface" | awk '{print $1}' | while read disk; do
    hdiutil detach "$disk" -force || true
done

if [ -f ./Subsurface-$CANONICALVERSION.dmg ]; then
	rm ./Subsurface-$CANONICALVERSION.dmg.bak
	mv ./Subsurface-$CANONICALVERSION.dmg ./Subsurface-$CANONICALVERSION.dmg.bak
fi

sleep 1

$DMGCREATE --background ${DIR}/subsurface/packaging/macosx/DMG-Background.png \
	--window-size 500 300 --icon-size 96 --volname Subsurface-$CANONICALVERSION \
	--app-drop-link 380 205 \
	--volicon ${DIR}/subsurface/packaging/macosx/Subsurface.icns \
	--icon "Subsurface" 110 205 ./Subsurface-$CANONICALVERSION.dmg ./staging

hdiutil detach /Volumes/Subsurface* -force 2>/dev/null || true
