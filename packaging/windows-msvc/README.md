# Building Subsurface on Windows with MSVC

This guide explains how to build Subsurface natively on Windows using Microsoft Visual Studio and vcpkg for dependency management.

## Prerequisites

### Required Software

1. **Visual Studio 2022** (Community, Professional, or Enterprise)
   - Install with "Desktop development with C++" workload
   - Ensure CMake and Ninja are included (or install separately)

2. **Git for Windows**
   - Download from https://git-scm.com/download/win

3. **Qt 6.8** (or later LTS)
   - Download the online installer from https://www.qt.io/download-qt-installer
   - Install Qt 6.8.x for MSVC 2022 64-bit
   - Include these modules: Qt Location, Qt Positioning, Qt Connectivity, Qt5Compat, Qt WebChannel, Qt WebEngine

4. **vcpkg** (for C/C++ dependencies)
   - If not already installed, see "Installing vcpkg" below

5. **NSIS** (for creating installers)
   - Download from https://nsis.sourceforge.io/Download
   - Add to PATH: `C:\Program Files (x86)\NSIS`

### Installing vcpkg

If you don't have vcpkg installed:

```powershell
cd C:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

Set the environment variable:
```powershell
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
```

## Quick Start

1. Clone the repository with submodules:
   ```powershell
   git clone --recursive https://github.com/subsurface/subsurface.git
   cd subsurface
   ```

2. Install dependencies:
   ```powershell
   .\packaging\windows-msvc\setup-dependencies.ps1
   ```

3. Set up the MSVC environment (or run from Developer Command Prompt):
   ```powershell
   . .\packaging\windows-msvc\setup-msvc-env.ps1
   ```

4. Build Subsurface:
   ```powershell
   .\packaging\windows-msvc\build.ps1
   ```

5. Install and create installer:
   ```powershell
   cd build-msvc
   ninja install
   ninja installer
   ```

   The installer `subsurface-<version>.exe` will be created in the build directory.

## Script Options

### setup-dependencies.ps1

Installs all required vcpkg dependencies. Run this once, or after updating dependencies.

```powershell
.\packaging\windows-msvc\setup-dependencies.ps1 [-VcpkgRoot <path>] [-Triplet <triplet>]
```

Options:
- `-VcpkgRoot`: Path to vcpkg installation (default: `C:\vcpkg` or `$env:VCPKG_ROOT`)
- `-Triplet`: vcpkg triplet (default: `x64-windows`)

### setup-msvc-env.ps1

Sets up the MSVC compiler environment in your current PowerShell session. Source this script (note the dot):

```powershell
. .\packaging\windows-msvc\setup-msvc-env.ps1 [-Arch <arch>]
```

Options:
- `-Arch`: Target architecture (default: `x64`)

### build.ps1

Builds Subsurface including libdivecomputer and googlemaps plugin.

```powershell
.\packaging\windows-msvc\build.ps1 [options]
```

Options:
- `-BuildType`: Build configuration: `Debug` or `Release` (default: `Release`)
- `-VcpkgRoot`: Path to vcpkg installation (auto-detected)
- `-Qt6Dir`: Path to Qt6 installation (auto-detected)
- `-SourceDir`: Path to Subsurface source (default: repository root)
- `-BuildDir`: Path for build output (default: `build-msvc`)
- `-Clean`: Remove build directories before building
- `-SkipLibdivecomputer`: Skip building libdivecomputer (if already built)
- `-SkipGooglemaps`: Skip building the googlemaps plugin
- `-Jobs`: Number of parallel build jobs (default: number of CPUs)

## Manual Build Steps

If you prefer to run commands manually:

### 1. Install vcpkg dependencies

```powershell
$env:VCPKG_ROOT = "C:\vcpkg"
& "$env:VCPKG_ROOT\vcpkg.exe" install `
    libxml2 libxslt libzip sqlite3 libgit2 libssh2 `
    curl[ssl] openssl libusb hidapi libraw `
    --triplet x64-windows
```

### 2. Build libdivecomputer

Open "x64 Native Tools Command Prompt for VS 2022" and run:

```powershell
# Install MSYS2 if needed for autotools (or use pre-generated revision.h)
# Generate revision.h using MSYS2:
# autoreconf --install --force
# ./configure
# make -C src revision.h

# Build with MSBuild
msbuild -m -p:Platform=x64 -p:Configuration=Release `
    libdivecomputer\contrib\msvc\libdivecomputer.vcxproj

# Copy to install directory
mkdir -p install\bin, install\lib, install\include\libdivecomputer
copy libdivecomputer\contrib\msvc\x64\Release\bin\*.dll install\bin\
copy libdivecomputer\contrib\msvc\x64\Release\bin\*.lib install\lib\
copy libdivecomputer\include\libdivecomputer\* install\include\libdivecomputer\
```

### 3. Build googlemaps plugin

```powershell
git clone https://github.com/Subsurface/googlemaps.git
cd googlemaps
git checkout qt6-upstream
mkdir build && cd build
qmake "CONFIG+=release" ..\googlemaps.pro
nmake
nmake install
cd ..\..
```

### 4. Configure and build Subsurface

```powershell
mkdir build && cd build

cmake .. -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$env:Qt6_DIR;$PWD\..\install;$env:VCPKG_ROOT\installed\x64-windows" `
    -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
    -DSUBSURFACE_TARGET_EXECUTABLE=DesktopExecutable `
    -DBUILD_WITH_QT6=ON `
    -DLIBDIVECOMPUTER_INCLUDE_DIR="$PWD\..\install\include" `
    -DLIBDIVECOMPUTER_LIBRARIES="$PWD\..\install\lib\divecomputer.lib" `
    -DLIBGIT2_INCLUDE_DIR="$env:VCPKG_ROOT\installed\x64-windows\include" `
    -DLIBGIT2_LIBRARIES="$env:VCPKG_ROOT\installed\x64-windows\lib\git2.lib" `
    -DMAKE_TESTS=OFF `
    -DNO_USERMANUAL=ON

ninja
```

### 5. Install and create installer

The `ninja install` target handles all DLL deployment automatically:
- Copies `subsurface.exe` and resources to the staging directory
- Copies Qt plugins (platforms, styles, imageformats, etc.)
- Copies vcpkg DLLs (libxml2, libxslt, libzip, etc.)
- Copies libdivecomputer DLL
- Copies MSVC runtime DLLs

```powershell
# Install everything to the staging directory
ninja install

# Create the NSIS installer
ninja installer
```

The installer will be created at `build-msvc/subsurface-<version>.exe`.

## Version Number

The version number is determined by `scripts/get-version.ps1`, which is the PowerShell equivalent of
`scripts/get-version.sh`. It reads the base version from `scripts/subsurface-base-version.txt` and
combines it with the build number from the `nightly-builds` repository.

To check your version:
```powershell
.\scripts\get-version.ps1
# Output: 6.0.5541-patch.59.local (example)
```

## Troubleshooting

### "Could NOT find LibXml2"
Make sure vcpkg dependencies are installed and the toolchain file is specified correctly.

### "pkg-config not found"
This is expected on Windows. The CMakeLists.txt has been updated to find libraries without pkg-config when building with MSVC.

### Qt not found
Set the `Qt6_DIR` environment variable to your Qt installation, e.g.:
```powershell
$env:Qt6_DIR = "C:\Qt\6.8.0\msvc2022_64"
```

### libdivecomputer build fails
Make sure you have the vcpkg include/lib paths in your INCLUDE and LIB environment variables, or run from the VS Developer Command Prompt.

### "No Qt platform plugin could be initialized"
This means Qt plugins are missing from the staging directory. The `ninja install` target should copy
these automatically. If they're missing, check that `QT_INSTALL_PREFIX` was correctly detected during
cmake configuration. You can also manually copy them:
```powershell
copy -Recurse "$env:Qt6_DIR\plugins\platforms" staging\plugins\
copy -Recurse "$env:Qt6_DIR\plugins\styles" staging\plugins\
```

### MSVC runtime DLLs missing (MSVCP140.dll, VCRUNTIME140.dll)
The `ninja install` target should bundle these automatically via `InstallRequiredSystemLibraries`.
If missing, you can install the Visual C++ Redistributable on the target machine, or manually copy
the DLLs from your Visual Studio installation.

## Directory Structure

After a successful build:
```
subsurface/
├── build-msvc/               # Build output
│   ├── staging/              # Files for installer
│   └── subsurface-*.exe      # Installer
├── install-msvc/             # libdivecomputer install
│   ├── bin/
│   ├── lib/
│   └── include/
├── googlemaps/               # googlemaps plugin source
└── packaging/windows-msvc/   # Build scripts
    ├── README.md             # This file
    ├── setup-dependencies.ps1  # Install vcpkg dependencies
    ├── setup-msvc-env.ps1    # Set up MSVC environment
    └── build.ps1             # Build everything
```
