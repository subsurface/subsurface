#Requires -Version 5.1
<#
.SYNOPSIS
    Get version string for Subsurface (Windows PowerShell equivalent of get-version.sh)

.DESCRIPTION
    Consistently name all builds, regardless of OS or desktop/mobile.
    Can create three digit (M.m.p) and four digit (M.m.p.c) version strings.
    Default is VERSION_EXTENSION version - an argument of '1', '3', or '4' gets you a digits only version string.

.PARAMETER Digits
    Optional: 1, 3, or 4 for digits-only version strings

.EXAMPLE
    .\get-version.ps1
    Returns: 6.0.123-local

.EXAMPLE
    .\get-version.ps1 4
    Returns: 6.0.123.0
#>

param(
    [ValidateSet("", "1", "3", "4")]
    [string]$Digits = ""
)

$ErrorActionPreference = "Stop"

# Figure out where we are
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$SubsurfaceSource = (Get-Item "$ScriptDir\..").FullName

# Read base version
$BaseVersionFile = Join-Path $SubsurfaceSource "scripts\subsurface-base-version.txt"
$SubsurfaceBaseVersion = (Get-Content $BaseVersionFile -Raw).Trim()

$CommitsSince = 0
$BuildNr = 0

# Check for build number file
$BuildNumberFile = Join-Path $SubsurfaceSource "latest-subsurface-buildnumber"
if (Test-Path $BuildNumberFile) {
    $BuildNr = [int](Get-Content $BuildNumberFile -Raw).Trim()
} else {
    # Try to get from nightly-builds repo
    $NightlyBuildsDir = Join-Path $SubsurfaceSource "nightly-builds"

    if (-not (Test-Path $NightlyBuildsDir)) {
        # Clone nightly-builds repo
        # Use Start-Process or call operator for paths with spaces
        try {
            & git clone "https://github.com/subsurface/nightly-builds" "$NightlyBuildsDir" 2>&1 | Out-Null
        } catch {
            # Ignore clone errors
        }
    }

    if (Test-Path $NightlyBuildsDir) {
        Push-Location $NightlyBuildsDir
        try {
            & git fetch 2>&1 | Out-Null

            # Get list of build branches sorted by commit date
            $branchOutput = & git branch -a --sort=-committerdate --list 2>&1
            $branches = @()
            foreach ($line in $branchOutput) {
                $line = $line.Trim()
                if ($line -match "remotes/origin/(branch-for-.+)$") {
                    $branches += $Matches[1]
                }
            }

            $LastBuildBranch = $null
            $LastBuildSha = $null

            foreach ($branch in $branches) {
                # Branch format is: branch-for-<sha>
                # Extract the SHA (last component after splitting by -)
                $parts = $branch -split "-"
                $sha = $parts[-1]
                if ($sha -and $sha -match "^[0-9a-f]+$") {
                    # Use call operator and proper argument passing
                    & git -C "$SubsurfaceSource" merge-base --is-ancestor $sha HEAD 2>&1 | Out-Null
                    if ($LASTEXITCODE -eq 0) {
                        $LastBuildBranch = $branch
                        $LastBuildSha = $sha
                        break
                    }
                }
            }

            if ($LastBuildBranch) {
                # Use try/catch around checkout since git writes to stderr even on success
                try {
                    $null = & git checkout $LastBuildBranch 2>&1
                } catch {
                    # Ignore - git writes "Already on..." to stderr which PowerShell treats as error
                }
                $BuildNrFile = Join-Path $NightlyBuildsDir "latest-subsurface-buildnumber"
                if (Test-Path $BuildNrFile) {
                    $content = (Get-Content $BuildNrFile -Raw).Trim()
                    if ($content -match '^\d+$') {
                        $BuildNr = [int]$content
                    }
                }

                # Count commits since last build
                $logOutput = & git -C "$SubsurfaceSource" log --pretty="oneline" "${LastBuildSha}...HEAD" 2>&1
                if ($logOutput) {
                    # Count lines properly - handle both array and string cases
                    if ($logOutput -is [array]) {
                        $CommitsSince = $logOutput.Count
                    } else {
                        $CommitsSince = 1
                    }
                }
            }
        } catch {
            # Ignore errors, use defaults
        }
        Pop-Location
    }
}

# Build version extension
$VersionExtension = "-"
if ($CommitsSince -ne 0) {
    $VersionExtension += "patch.${CommitsSince}."
}

$ExtensionFile = Join-Path $SubsurfaceSource "latest-subsurface-buildnumber-extension"
if (Test-Path $ExtensionFile) {
    $Suffix = (Get-Content $ExtensionFile -Raw).Trim()
    # Sanitize: replace / and _ with -, remove other special chars
    $Suffix = $Suffix -replace "[/_]", "-"
    $Suffix = $Suffix -replace "[^.a-zA-Z0-9-]", ""
    $VersionExtension += $Suffix
} else {
    $VersionExtension += "local"
}

# Generate version based on digits parameter
switch ($Digits) {
    "1" { $Version = "$BuildNr" }
    "3" { $Version = "${SubsurfaceBaseVersion}.${BuildNr}" }
    "4" { $Version = "${SubsurfaceBaseVersion}.${BuildNr}.${CommitsSince}" }
    default { $Version = "${SubsurfaceBaseVersion}.${BuildNr}${VersionExtension}" }
}

# Output to stdout without any newline or CRLF
# Use .NET Console.Out.Write for clean output that CMake can capture
[Console]::Out.Write($Version)
