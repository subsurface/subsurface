#Requires -Version 5.1
<#
.SYNOPSIS
    Installs vcpkg dependencies for building Subsurface on Windows with MSVC.

.DESCRIPTION
    This script installs all the C/C++ library dependencies needed to build
    Subsurface using vcpkg. Run this once before building, or after updating
    the dependency list.

.PARAMETER VcpkgRoot
    Path to the vcpkg installation. Defaults to $env:VCPKG_ROOT or C:\vcpkg.

.PARAMETER Triplet
    The vcpkg triplet to use. Defaults to x64-windows.

.EXAMPLE
    .\setup-dependencies.ps1

.EXAMPLE
    .\setup-dependencies.ps1 -VcpkgRoot D:\tools\vcpkg
#>

param(
    [string]$VcpkgRoot = "",
    [string]$Triplet = "x64-windows"
)

$ErrorActionPreference = "Stop"

# Find vcpkg
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

To install vcpkg:
    cd C:\
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    .\bootstrap-vcpkg.bat
"@
        exit 1
    }
}

$vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path $vcpkgExe)) {
    Write-Error "vcpkg.exe not found at $vcpkgExe. Please run bootstrap-vcpkg.bat first."
    exit 1
}

Write-Host "Using vcpkg at: $VcpkgRoot" -ForegroundColor Cyan
Write-Host "Triplet: $Triplet" -ForegroundColor Cyan
Write-Host ""

# List of dependencies
$dependencies = @(
    "libxml2",
    "libxslt",
    "libzip",
    "sqlite3",
    "libgit2",
    "libssh2",
    "curl[ssl]",
    "openssl",
    "libusb",
    "hidapi",
    "libraw"
)

Write-Host "Installing dependencies:" -ForegroundColor Green
$dependencies | ForEach-Object { Write-Host "  - $_" }
Write-Host ""

# Install dependencies
$args = @("install") + $dependencies + @("--triplet", $Triplet)
Write-Host "Running: vcpkg $($args -join ' ')" -ForegroundColor Yellow
Write-Host ""

& $vcpkgExe @args

if ($LASTEXITCODE -ne 0) {
    Write-Error "vcpkg install failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "Dependencies installed successfully!" -ForegroundColor Green
Write-Host ""

# List installed packages
Write-Host "Installed packages:" -ForegroundColor Cyan
& $vcpkgExe list

Write-Host ""
Write-Host "Next steps:" -ForegroundColor Green
Write-Host "  1. Make sure Qt 6.8 is installed and Qt6_DIR is set"
Write-Host "  2. Run .\packaging\windows-msvc\build.ps1 to build Subsurface"
