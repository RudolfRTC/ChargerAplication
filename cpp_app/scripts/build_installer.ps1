#Requires -Version 5.1
<#
.SYNOPSIS
    Build the distributable and then create an Inno Setup installer.

.DESCRIPTION
    1. Calls make_dist.ps1 to build and deploy into dist/.
    2. Runs ISCC.exe (Inno Setup Compiler) to produce the installer .exe.

.PARAMETER QtPrefix
    Forwarded to make_dist.ps1 (optional Qt path).

.PARAMETER IsccPath
    Path to ISCC.exe. If not set, checks $env:ISCC_PATH, then common
    install locations.

.EXAMPLE
    .\scripts\build_installer.ps1
    .\scripts\build_installer.ps1 -QtPrefix "C:\Qt\6.10.2\mingw_64"
    $env:ISCC_PATH = "D:\InnoSetup\ISCC.exe"; .\scripts\build_installer.ps1
#>
param(
    [string]$QtPrefix = "",
    [string]$IsccPath = ""
)

$ErrorActionPreference = "Stop"
$ScriptDir  = $PSScriptRoot
$ProjectDir = Split-Path -Parent $ScriptDir   # cpp_app/

# ---- 1. Build dist ----
Write-Host "=== Step 1: Building distributable ===" -ForegroundColor Cyan

$distArgs = @()
if ($QtPrefix -ne "") {
    $distArgs += "-QtPrefix", $QtPrefix
}
& "$ScriptDir\make_dist.ps1" @distArgs

# ---- 2. Find ISCC.exe ----
Write-Host "`n=== Step 2: Creating installer ===" -ForegroundColor Cyan

if ($IsccPath -eq "") {
    $IsccPath = $env:ISCC_PATH
}

if ($IsccPath -eq "" -or -not (Test-Path $IsccPath -ErrorAction SilentlyContinue)) {
    # Search common locations
    $candidates = @(
        "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
        "C:\Program Files\Inno Setup 6\ISCC.exe",
        "C:\Program Files (x86)\Inno Setup 5\ISCC.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) {
            $IsccPath = $c
            break
        }
    }
}

# Also try PATH
if ($IsccPath -eq "" -or -not (Test-Path $IsccPath -ErrorAction SilentlyContinue)) {
    $fromPath = Get-Command "ISCC" -ErrorAction SilentlyContinue
    if ($fromPath) {
        $IsccPath = $fromPath.Source
    }
}

if ($IsccPath -eq "" -or -not (Test-Path $IsccPath -ErrorAction SilentlyContinue)) {
    Write-Host @"

ERROR: Inno Setup Compiler (ISCC.exe) not found.

To fix this, do one of the following:
  1. Install Inno Setup 6 from: https://jrsoftware.org/isdl.php
     Default install path: C:\Program Files (x86)\Inno Setup 6\ISCC.exe

  2. Set the ISCC_PATH environment variable:
       `$env:ISCC_PATH = "C:\Path\To\ISCC.exe"

  3. Pass it as a parameter:
       .\scripts\build_installer.ps1 -IsccPath "C:\Path\To\ISCC.exe"

  4. Add ISCC.exe to your PATH.
"@ -ForegroundColor Red
    exit 1
}

Write-Host "Using ISCC: $IsccPath" -ForegroundColor Green

# ---- 3. Create output directory ----
$OutputDir = Join-Path $ProjectDir "output"
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

# ---- 4. Run Inno Setup ----
$IssFile = Join-Path $ProjectDir "installer\installer.iss"
& $IsccPath $IssFile
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup compilation failed."
}

# ---- Done ----
Write-Host "`n=== Installer created ===" -ForegroundColor Green
Get-ChildItem $OutputDir -Filter "*.exe" | ForEach-Object {
    Write-Host "  $($_.FullName)" -ForegroundColor Cyan
    Write-Host "  Size: $([math]::Round($_.Length / 1MB, 2)) MB"
}
