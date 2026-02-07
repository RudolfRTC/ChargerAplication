# Windows Build & Distribution Guide

## Prerequisites

| Tool | Required | Notes |
|------|----------|-------|
| **Qt 6 MinGW kit** | Yes | e.g. `C:\Qt\6.10.2\mingw_64` |
| **MinGW** | Yes | Usually bundled with Qt (`Tools\mingw...`) |
| **CMake** | Yes | 3.16+ (bundled with Qt or standalone) |
| **windeployqt** | Yes | Ships with Qt (in the `bin/` folder) |
| **Inno Setup 6** | Optional | Only for building the installer |

Make sure `cmake`, `mingw32-make`, and your Qt `bin/` directory are in your `PATH`:

```powershell
# Example: add Qt MinGW to PATH for the current session
$env:PATH += ";C:\Qt\6.10.2\mingw_64\bin"
$env:PATH += ";C:\Qt\Tools\mingw1310_64\bin"
```

## Quick Start

### 1. Build + Distributable Folder

```powershell
cd cpp_app
.\scripts\make_dist.ps1
```

Or specify the Qt path explicitly:

```powershell
.\scripts\make_dist.ps1 -QtPrefix "C:\Qt\6.10.2\mingw_64"
```

This will:
1. Configure CMake (Release, MinGW Makefiles)
2. Build the project
3. Create `dist/` with the executable
4. Run `windeployqt` to bundle all Qt DLLs and plugins
5. Optionally copy `PCANBasic.dll` if found

The `dist/` folder is self-contained and can be copied to any Windows PC
(no Qt installation required on the target machine).

### 2. Build Installer (.exe)

```powershell
.\scripts\build_installer.ps1
```

Or with explicit paths:

```powershell
.\scripts\build_installer.ps1 -QtPrefix "C:\Qt\6.10.2\mingw_64"
```

This will:
1. Run `make_dist.ps1` (build + deploy)
2. Run Inno Setup Compiler (`ISCC.exe`) to produce the installer

The installer `.exe` will be in `output/`.

If Inno Setup is not found, you can set its path:

```powershell
$env:ISCC_PATH = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
.\scripts\build_installer.ps1
```

## What the Installer Does

- Installs into `C:\Program Files\OBCChargerController`
- Creates Start Menu shortcut
- Optional Desktop shortcut
- Uses the application icon for all shortcuts
- Includes uninstaller

## PCAN Support (Optional)

If you use PEAK PCAN-Basic CAN adapters, place `PCANBasic.dll` in one of
these locations (the build script checks them in order):

1. Next to the build script (`scripts/PCANBasic.dll`)
2. In the project root (`cpp_app/PCANBasic.dll`)
3. `C:\Windows\System32\PCANBasic.dll` (installed by PEAK driver)
4. `C:\Program Files\PEAK-System\PCAN-Basic API\x64\PCANBasic.dll`

Or pass it explicitly:

```powershell
.\scripts\make_dist.ps1 -PcanDll "D:\Drivers\PCANBasic.dll"
```

If `PCANBasic.dll` is not bundled, the application will still work in
SocketCAN (Linux) and Simulator modes. On Windows, attempting to use the
PCAN backend without the DLL will show a clear error message.

## Manual Build (without scripts)

```powershell
cd cpp_app

# Configure
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_PREFIX_PATH="C:\Qt\6.10.2\mingw_64"

# Build
cmake --build build

# Deploy
mkdir dist
copy build\OBC_Charger_Controller.exe dist\
windeployqt --release --compiler-runtime dist\OBC_Charger_Controller.exe
```

## Linux Build (unchanged)

```bash
cd cpp_app
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

Requires: `qt6-base-dev`, `libqt6charts6-dev`

## Project Structure (Windows-related files)

```
cpp_app/
  assets/
    logo.png          # Company logo (splash screen, embedded via QRC)
    logocon.png       # App icon source (PNG)
    app_icon.ico      # App icon (multi-size ICO: 16/32/48/256)
  app.rc              # Windows resource file (embeds ICO into .exe)
  resources.qrc       # Qt resource file (embeds logo.png)
  scripts/
    make_dist.ps1     # Build + windeployqt -> dist/
    build_installer.ps1  # Build + Inno Setup -> installer .exe
  installer/
    installer.iss     # Inno Setup script
  dist/               # (generated) Distributable folder
  output/             # (generated) Installer output
```
