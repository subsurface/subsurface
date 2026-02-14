#Requires -Version 5.1
<#
.SYNOPSIS
    Sets up the MSVC build environment in the current PowerShell session.

.DESCRIPTION
    This script finds Visual Studio and sets up the MSVC compiler environment
    variables so you can build without opening the Developer Command Prompt.

.PARAMETER Arch
    Target architecture. Defaults to x64.

.EXAMPLE
    . .\setup-msvc-env.ps1
    # Now you can run build.ps1 in the same session
#>

param(
    [string]$Arch = "x64"
)

# Find vswhere
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Error "vswhere not found. Is Visual Studio installed?"
    return
}

# Find Visual Studio
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    Write-Error "Visual Studio with C++ tools not found."
    return
}

Write-Host "Found Visual Studio at: $vsPath" -ForegroundColor Cyan

# Find vcvarsall.bat
$vcvarsall = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
if (-not (Test-Path $vcvarsall)) {
    Write-Error "vcvarsall.bat not found at $vcvarsall"
    return
}

# Run vcvarsall and capture environment
Write-Host "Setting up $Arch build environment..." -ForegroundColor Cyan

$envBefore = @{}
Get-ChildItem env: | ForEach-Object { $envBefore[$_.Name] = $_.Value }

# Run vcvarsall in cmd and capture the resulting environment
$tempFile = [System.IO.Path]::GetTempFileName()
cmd /c "`"$vcvarsall`" $Arch && set > `"$tempFile`""

if ($LASTEXITCODE -ne 0) {
    Write-Error "vcvarsall.bat failed"
    Remove-Item $tempFile -ErrorAction SilentlyContinue
    return
}

# Parse and apply the new environment
Get-Content $tempFile | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') {
        $name = $matches[1]
        $value = $matches[2]
        if ($envBefore[$name] -ne $value) {
            Set-Item -Path "env:$name" -Value $value
        }
    }
}

Remove-Item $tempFile

Write-Host ""
Write-Host "MSVC environment configured for $Arch" -ForegroundColor Green
Write-Host ""
Write-Host "You can now run:" -ForegroundColor Cyan
Write-Host "  .\build.ps1" -ForegroundColor White
