#Requires -Version 5.1
<#
.SYNOPSIS
    Build OBC Charger Controller (Release) and create a distributable folder.

.DESCRIPTION
    1. Configures CMake with MinGW Makefiles (Release).
    2. Builds the project.
    3. Creates a dist/ folder with the executable.
    4. Runs windeployqt to copy all required Qt DLLs/plugins.
    5. Optionally copies PCANBasic.dll if available.

.PARAMETER QtPrefix
    Path to the Qt MinGW kit (e.g. C:\Qt\6.10.2\mingw_64).
    If not set, CMake will try to find Qt from PATH / CMAKE_PREFIX_PATH.

.PARAMETER PcanDll
    Optional path to PCANBasic.dll. If the file exists it is copied into dist/.

.EXAMPLE
    .\scripts\make_dist.ps1
    .\scripts\make_dist.ps1 -QtPrefix "C:\Qt\6.10.2\mingw_64"
#>
param(
    [string]$QtPrefix = "",
    [string]$PcanDll  = ""
)

$ErrorActionPreference = "Stop"
$ProjectDir = Split-Path -Parent $PSScriptRoot   # cpp_app/
$BuildDir   = Join-Path $ProjectDir "build"
$DistDir    = Join-Path $ProjectDir "dist"
$ExeName    = "OBC_Charger_Controller.exe"

Write-Host "=== OBC Charger Controller - Build & Deploy ===" -ForegroundColor Cyan

# ---- 1. Configure ----
Write-Host "`n[1/4] Configuring CMake (Release, MinGW Makefiles)..." -ForegroundColor Yellow

$cmakeArgs = @(
    "-S", $ProjectDir,
    "-B", $BuildDir,
    "-G", "MinGW Makefiles",
    "-DCMAKE_BUILD_TYPE=Release"
)
if ($QtPrefix -ne "") {
    $cmakeArgs += "-DCMAKE_PREFIX_PATH=$QtPrefix"
}

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

# ---- 2. Build ----
Write-Host "`n[2/4] Building..." -ForegroundColor Yellow
& cmake --build $BuildDir --config Release
if ($LASTEXITCODE -ne 0) { throw "Build failed." }

$ExePath = Join-Path $BuildDir $ExeName
if (-not (Test-Path $ExePath)) {
    throw "Built executable not found at: $ExePath"
}

# ---- 3. Create dist/ ----
Write-Host "`n[3/4] Creating dist/ folder..." -ForegroundColor Yellow
if (Test-Path $DistDir) {
    Remove-Item -Recurse -Force $DistDir
}
New-Item -ItemType Directory -Path $DistDir | Out-Null
Copy-Item $ExePath $DistDir

# ---- 4. windeployqt ----
Write-Host "`n[4/4] Running windeployqt..." -ForegroundColor Yellow

$windeployqt = Get-Command "windeployqt" -ErrorAction SilentlyContinue
if (-not $windeployqt) {
    # Try to find it relative to QtPrefix
    if ($QtPrefix -ne "") {
        $candidate = Join-Path $QtPrefix "bin\windeployqt.exe"
        if (Test-Path $candidate) {
            $windeployqt = $candidate
        }
    }
}
if (-not $windeployqt) {
    Write-Host @"

ERROR: windeployqt not found in PATH.
       Add your Qt bin directory to PATH, for example:
         `$env:PATH += ";C:\Qt\6.10.2\mingw_64\bin"
       Then re-run this script.
"@ -ForegroundColor Red
    exit 1
}

$deployExe = Join-Path $DistDir $ExeName
& $windeployqt --release --compiler-runtime $deployExe
if ($LASTEXITCODE -ne 0) {
    Write-Host "WARNING: windeployqt returned non-zero. Check output above." -ForegroundColor Yellow
}

# ---- 5. PCANBasic.dll (optional) ----
$pcanCandidates = @(
    $PcanDll,
    (Join-Path $ProjectDir "PCANBasic.dll"),
    (Join-Path $PSScriptRoot "PCANBasic.dll"),
    "C:\Windows\System32\PCANBasic.dll",
    "C:\Program Files\PEAK-System\PCAN-Basic API\x64\PCANBasic.dll"
)
$pcanCopied = $false
foreach ($p in $pcanCandidates) {
    if ($p -ne "" -and (Test-Path $p)) {
        Copy-Item $p (Join-Path $DistDir "PCANBasic.dll")
        Write-Host "Copied PCANBasic.dll from: $p" -ForegroundColor Green
        $pcanCopied = $true
        break
    }
}
if (-not $pcanCopied) {
    Write-Host @"
NOTE: PCANBasic.dll not found. If you need PCAN support, copy PCANBasic.dll
      into the dist/ folder manually, or install the PEAK PCAN-Basic driver.
"@ -ForegroundColor Yellow
}

# ---- Done ----
Write-Host "`n=== Done ===" -ForegroundColor Green
Write-Host "Distributable folder: $DistDir"
Write-Host "Files:" -ForegroundColor Cyan
Get-ChildItem $DistDir -Recurse | ForEach-Object {
    $rel = $_.FullName.Substring($DistDir.Length + 1)
    Write-Host "  $rel"
}
