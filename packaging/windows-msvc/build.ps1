#Requires -Version 5.1
<#
.SYNOPSIS
    Builds Subsurface on Windows with MSVC and Qt6.

.DESCRIPTION
    This script builds Subsurface including libdivecomputer and the googlemaps
    plugin. It assumes vcpkg dependencies are already installed (run
    setup-dependencies.ps1 first).

.PARAMETER VcpkgRoot
    Path to the vcpkg installation. Defaults to $env:VCPKG_ROOT or C:\vcpkg.

.PARAMETER Qt6Dir
    Path to Qt6 installation. Defaults to $env:Qt6_DIR.

.PARAMETER BuildType
    Build type: Debug or Release. Defaults to Release.

.PARAMETER SourceDir
    Path to Subsurface source. Defaults to the repository root.

.PARAMETER BuildDir
    Path for build output. Defaults to <SourceDir>\build-msvc.

.PARAMETER Clean
    If specified, removes the build directory before building.

.PARAMETER SkipLibdivecomputer
    Skip building libdivecomputer (use if already built).

.PARAMETER SkipGooglemaps
    Skip building the googlemaps plugin.

.PARAMETER Jobs
    Number of parallel build jobs. Defaults to number of processors.

.EXAMPLE
    .\build.ps1

.EXAMPLE
    .\build.ps1 -BuildType Debug -Clean

.EXAMPLE
    .\build.ps1 -Qt6Dir "C:\Qt\6.8.0\msvc2022_64"
#>

param(
    [string]$VcpkgRoot = "",
    [string]$Qt6Dir = "",
    [string]$BuildType = "Release",
    [string]$SourceDir = "",
    [string]$BuildDir = "",
    [switch]$Clean,
    [switch]$SkipLibdivecomputer,
    [switch]$SkipGooglemaps,
    [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------
# Helper functions
# ---------------------------------------------------------------

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host "===============================================================================" -ForegroundColor Cyan
    Write-Host " $Message" -ForegroundColor Cyan
    Write-Host "===============================================================================" -ForegroundColor Cyan
    Write-Host ""
}

function Test-Command {
    param([string]$Command)
    $null = Get-Command $Command -ErrorAction SilentlyContinue
    return $?
}

# ---------------------------------------------------------------
# Determine paths
# ---------------------------------------------------------------

# Source directory (repository root)
if (-not $SourceDir) {
    $SourceDir = (Get-Item "$PSScriptRoot\..\..").FullName
}

# Build directory
if (-not $BuildDir) {
    $BuildDir = Join-Path $SourceDir "build-msvc"
}

# Install directory (for libdivecomputer)
$InstallDir = Join-Path $SourceDir "install-msvc"

# Jobs
if ($Jobs -le 0) {
    $Jobs = $env:NUMBER_OF_PROCESSORS
    if (-not $Jobs) { $Jobs = 4 }
}

# ---------------------------------------------------------------
# Find vcpkg
# ---------------------------------------------------------------

if (-not $VcpkgRoot) {
    if ($env:VCPKG_ROOT) {
        $VcpkgRoot = $env:VCPKG_ROOT
    } elseif (Test-Path "C:\vcpkg") {
        $VcpkgRoot = "C:\vcpkg"
    } else {
        Write-Error @"
vcpkg not found. Please either:
1. Set the VCPKG_ROOT environment variable
2. Install vcpkg to C:\vcpkg
3. Specify -VcpkgRoot parameter

Run .\setup-dependencies.ps1 after installing vcpkg.
"@
        exit 1
    }
}

$vcpkgInstalled = Join-Path $VcpkgRoot "installed\x64-windows"
if (-not (Test-Path $vcpkgInstalled)) {
    Write-Error @"
vcpkg dependencies not installed. Run:
    .\setup-dependencies.ps1
"@
    exit 1
}

# ---------------------------------------------------------------
# Find Qt6
# ---------------------------------------------------------------

if (-not $Qt6Dir) {
    if ($env:Qt6_DIR) {
        $Qt6Dir = $env:Qt6_DIR
    } else {
        # Try common Qt installation paths
        $qtPaths = @(
            "C:\Qt\6.8*\msvc2022_64",
            "C:\Qt\6.7*\msvc2022_64",
            "C:\Qt\6.6*\msvc2022_64",
            "$env:USERPROFILE\Qt\6.8*\msvc2022_64",
            "$env:USERPROFILE\Qt\6.7*\msvc2022_64"
        )
        foreach ($pattern in $qtPaths) {
            $found = Get-ChildItem -Path $pattern -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($found) {
                $Qt6Dir = $found.FullName
                break
            }
        }
    }
}

if (-not $Qt6Dir -or -not (Test-Path $Qt6Dir)) {
    Write-Error @"
Qt6 not found. Please either:
1. Set the Qt6_DIR environment variable
2. Install Qt 6.8 to C:\Qt
3. Specify -Qt6Dir parameter

Install Qt from: https://www.qt.io/download-qt-installer
"@
    exit 1
}

# ---------------------------------------------------------------
# Verify tools
# ---------------------------------------------------------------

Write-Step "Checking prerequisites"

Write-Host "Source directory: $SourceDir"
Write-Host "Build directory:  $BuildDir"
Write-Host "Install directory: $InstallDir"
Write-Host "vcpkg:            $VcpkgRoot"
Write-Host "Qt6:              $Qt6Dir"
Write-Host "Build type:       $BuildType"
Write-Host "Parallel jobs:    $Jobs"
Write-Host ""

# Check for required tools
$missingTools = @()

if (-not (Test-Command "cmake")) {
    $missingTools += "cmake"
}
if (-not (Test-Command "ninja")) {
    $missingTools += "ninja"
}
if (-not (Test-Command "msbuild")) {
    $missingTools += "msbuild (run from Developer Command Prompt or use setup-msvc.ps1)"
}

if ($missingTools.Count -gt 0) {
    Write-Error "Missing required tools: $($missingTools -join ', ')"
    exit 1
}

Write-Host "All prerequisites found." -ForegroundColor Green

# ---------------------------------------------------------------
# Clean if requested
# ---------------------------------------------------------------

if ($Clean) {
    Write-Step "Cleaning build directories"
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
        Write-Host "Removed $BuildDir"
    }
    if (Test-Path $InstallDir) {
        Remove-Item -Recurse -Force $InstallDir
        Write-Host "Removed $InstallDir"
    }
}

# Create directories
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
New-Item -ItemType Directory -Force -Path "$InstallDir\bin" | Out-Null
New-Item -ItemType Directory -Force -Path "$InstallDir\lib" | Out-Null
New-Item -ItemType Directory -Force -Path "$InstallDir\include" | Out-Null

# ---------------------------------------------------------------
# Build libdivecomputer
# ---------------------------------------------------------------

if (-not $SkipLibdivecomputer) {
    Write-Step "Building libdivecomputer"

    $libdcDir = Join-Path $SourceDir "libdivecomputer"
    $libdcProj = Join-Path $libdcDir "contrib\msvc\libdivecomputer.vcxproj"

    if (-not (Test-Path $libdcProj)) {
        Write-Error "libdivecomputer MSVC project not found at $libdcProj"
        exit 1
    }

    # Parse version info from configure.ac (following upstream's approach)
    $configureAc = Get-Content (Join-Path $libdcDir "configure.ac") -Raw

    if ($configureAc -match 'm4_define\(\[dc_version_major\],\[(\d+)\]\)') {
        $major = $matches[1]
    } else { $major = "0" }

    if ($configureAc -match 'm4_define\(\[dc_version_minor\],\[(\d+)\]\)') {
        $minor = $matches[1]
    } else { $minor = "0" }

    if ($configureAc -match 'm4_define\(\[dc_version_micro\],\[(\d+)\]\)') {
        $micro = $matches[1]
    } else { $micro = "0" }

    if ($configureAc -match 'm4_define\(\[dc_version_suffix\],\[([^\]]*)\]\)') {
        $suffix = $matches[1]
        $version = "$major.$minor.$micro-$suffix"
    } else {
        $version = "$major.$minor.$micro"
    }

    Write-Host "Parsed version from configure.ac: $version" -ForegroundColor Green

    # Generate version.h from version.h.in template
    $versionHIn = Join-Path $libdcDir "include\libdivecomputer\version.h.in"
    $versionHOut = Join-Path $libdcDir "include\libdivecomputer\version.h"
    $versionH = Get-Content $versionHIn -Raw
    $versionH = $versionH -replace '@DC_VERSION@', $version
    $versionH = $versionH -replace '@DC_VERSION_MAJOR@', $major
    $versionH = $versionH -replace '@DC_VERSION_MINOR@', $minor
    $versionH = $versionH -replace '@DC_VERSION_MICRO@', $micro
    $versionH | Out-File -FilePath $versionHOut -Encoding utf8
    Write-Host "Generated version.h" -ForegroundColor Green

    # Generate revision.h with git commit hash
    $revisionH = Join-Path $libdcDir "src\revision.h"
    $gitHash = git -C $libdcDir rev-parse --verify HEAD 2>$null
    if ($gitHash) {
        "#define DC_VERSION_REVISION `"$gitHash`"" | Out-File -FilePath $revisionH -Encoding utf8
        Write-Host "Generated revision.h with revision: $gitHash" -ForegroundColor Green
    } else {
        Write-Error "Could not get git revision for libdivecomputer"
        exit 1
    }

    # Set up include/lib paths for vcpkg
    $env:INCLUDE = "$vcpkgInstalled\include;$env:INCLUDE"
    $env:LIB = "$vcpkgInstalled\lib;$env:LIB"

    # Suppress deprecation warnings
    $env:CL = "/WX- /wd4996 /D_CRT_NONSTDC_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS"

    Write-Host "Building with msbuild..."
    msbuild -m -p:Platform=x64 -p:Configuration=$BuildType $libdcProj

    if ($LASTEXITCODE -ne 0) {
        Write-Error "libdivecomputer build failed"
        exit $LASTEXITCODE
    }

    # Copy built files
    $outputDir = Join-Path $libdcDir "contrib\msvc\x64\$BuildType\bin"
    Write-Host "Copying built files from $outputDir"

    Get-ChildItem "$outputDir\*.dll" -ErrorAction SilentlyContinue | Copy-Item -Destination "$InstallDir\bin"
    Get-ChildItem "$outputDir\*.lib" -ErrorAction SilentlyContinue | Copy-Item -Destination "$InstallDir\lib"

    # Copy headers
    $includeDir = Join-Path $InstallDir "include\libdivecomputer"
    New-Item -ItemType Directory -Force -Path $includeDir | Out-Null
    Copy-Item "$libdcDir\include\libdivecomputer\*" -Destination $includeDir -Recurse -Force

    Write-Host "libdivecomputer built successfully" -ForegroundColor Green
}

# ---------------------------------------------------------------
# Build googlemaps plugin
# ---------------------------------------------------------------

if (-not $SkipGooglemaps) {
    Write-Step "Building googlemaps plugin"

    $googlemapsDir = Join-Path $SourceDir "googlemaps"

    # Clone if not present
    if (-not (Test-Path $googlemapsDir)) {
        Write-Host "Cloning googlemaps plugin..."
        git clone https://github.com/Subsurface/googlemaps.git $googlemapsDir
        Push-Location $googlemapsDir
        git checkout qt6-upstream
        Pop-Location
    }

    $googlemapsBuild = Join-Path $googlemapsDir "build"
    New-Item -ItemType Directory -Force -Path $googlemapsBuild | Out-Null

    Push-Location $googlemapsBuild

    # Build with qmake
    $qmake = Join-Path $Qt6Dir "bin\qmake.exe"
    if (-not (Test-Path $qmake)) {
        Write-Error "qmake not found at $qmake"
        Pop-Location
        exit 1
    }

    Write-Host "Running qmake..."
    & $qmake "CONFIG+=release" "$googlemapsDir\googlemaps.pro"

    if ($LASTEXITCODE -ne 0) {
        Write-Error "qmake failed"
        Pop-Location
        exit $LASTEXITCODE
    }

    Write-Host "Building with nmake..."
    nmake

    if ($LASTEXITCODE -ne 0) {
        Write-Error "googlemaps build failed"
        Pop-Location
        exit $LASTEXITCODE
    }

    Write-Host "Installing googlemaps plugin..."
    nmake install

    Pop-Location

    Write-Host "googlemaps plugin built successfully" -ForegroundColor Green
}

# ---------------------------------------------------------------
# Configure and build Subsurface
# ---------------------------------------------------------------

Write-Step "Configuring Subsurface"

Push-Location $BuildDir

$prefixPath = @(
    $Qt6Dir,
    $InstallDir,
    $vcpkgInstalled
) -join ";"

$cmakeArgs = @(
    $SourceDir,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_PREFIX_PATH=$prefixPath",
    "-DCMAKE_TOOLCHAIN_FILE=$VcpkgRoot\scripts\buildsystems\vcpkg.cmake",
    "-DSUBSURFACE_TARGET_EXECUTABLE=DesktopExecutable",
    "-DBUILD_WITH_QT6=ON",
    "-DLIBDIVECOMPUTER_INCLUDE_DIR=$InstallDir\include",
    "-DLIBDIVECOMPUTER_LIBRARIES=$InstallDir\lib\libdivecomputer.lib",
    "-DLIBGIT2_INCLUDE_DIR=$vcpkgInstalled\include",
    "-DLIBGIT2_LIBRARIES=$vcpkgInstalled\lib\git2.lib",
    "-DMAKE_TESTS=OFF",
    "-DNO_USERMANUAL=ON"
)

Write-Host "Running cmake..."
cmake @cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed"
    Pop-Location
    exit $LASTEXITCODE
}

Write-Step "Building Subsurface"

cmake --build . --config $BuildType -j $Jobs

if ($LASTEXITCODE -ne 0) {
    Write-Error "Subsurface build failed"
    Pop-Location
    exit $LASTEXITCODE
}

Pop-Location

Write-Step "Build complete!"

Write-Host "Subsurface has been built successfully." -ForegroundColor Green
Write-Host ""
Write-Host "Build output: $BuildDir" -ForegroundColor Cyan
Write-Host ""
Write-Host "To run Subsurface, you may need to copy DLLs to the build directory:"
Write-Host "  - Qt DLLs: Run windeployqt on subsurface.exe"
Write-Host "  - vcpkg DLLs: Copy from $vcpkgInstalled\bin"
Write-Host "  - libdivecomputer DLL: Copy from $InstallDir\bin"
Write-Host ""
Write-Host "To create an installer, run:"
Write-Host "  cmake --build $BuildDir --target installer"
