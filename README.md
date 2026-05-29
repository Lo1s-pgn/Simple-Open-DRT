# Simple Open DRT (OFX)

This is a port into an OFX plugin of [OpenDRT by Jed Smith](https://github.com/jedypod/open-display-transform), with a simplified UI, combined preset workflow, cross-platform GPU/CPU render paths and a refactored codebase.

Current OpenDRT version: `1.1.0`

## Platform

- Windows (x86_64): CUDA + OpenCL + CPU
- macOS (arm64 + x86_64): Metal + CPU
- Linux (x86_64): CUDA + OpenCL + CPU fallback

Build on each platform separately. Each build writes a **versioned, platform-tagged release folder** containing the `.ofx.bundle`:

| Platform | Release folder (example for v1.1.7) | Inside the bundle |
|----------|-------------------------------------|-------------------|
| macOS | `release/LSP_Simple_Open_DRT_1.1.7_macos/` | `Contents/MacOS/<name>.ofx` + `Resources/OpenDRT.metallib` |
| Windows | `release/LSP_Simple_Open_DRT_1.1.7_windows/` | `Contents/Win64/<name>.ofx` |
| Linux | `release/LSP_Simple_Open_DRT_1.1.7_linux/` | `Contents/Linux-x86-64/<name>.ofx` |

Pattern: **`LSP_Simple_Open_DRT_<version>_<platform>/`**

The bundle inside is always **`LSP_Simple_Open_DRT_<version>.ofx.bundle`**.

## Build

**CMake** is the only build system. See [tools/README.md](tools/README.md) for optional build helper scripts.

### Prerequisites

- **CMake** 3.22+
- **macOS:** Xcode Command Line Tools
- **Windows:** Visual Studio 2022 (MSVC), **Ninja**, **CUDA Toolkit** (CUDA + OpenCL)
- **Linux:** **Ninja**, OpenCL ICD dev headers (`ocl-icd-opencl-dev`), optionally **CUDA Toolkit** (enabled by default)

### macOS

```bash
./tools/macos/opendrt_build.sh
```

Or manually:

```bash
cmake -S . -B build/macos -DCMAKE_BUILD_TYPE=Release
cmake --build build/macos --target opendrt_all
```

Options at configure time:

- `-DOPENDRT_OFX_FAT_ARCHS=OFF` — single-arch build (host CPU only)
- `-DOFX_SDK_PATH=...` — alternate OpenFX SDK checkout

### Windows

```powershell
tools\windows\opendrt_build.bat
```

Or manually (from a **Developer Command Prompt** or after `vcvars64.bat`):

```powershell
cmake -S . -B build/windows -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl
cmake --build build/windows --target opendrt_all
```

Optional Windows installer: see [packaging/windows/README.md](packaging/windows/README.md).

### Linux

```bash
./tools/linux/opendrt_build.sh
```

Or manually:

```bash
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux --target opendrt_all
```

If you want CPU + OpenCL only (no CUDA):

```bash
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DSIMPLE_OPENDRT_LINUX_CUDA=OFF
cmake --build build/linux --target opendrt_all
```

## GitHub Actions

This repo includes **Build OFX release** (`.github/workflows/build-ofx-release.yml`). Trigger it manually from GitHub → Actions → Run workflow. It builds macOS, Windows, and Linux in parallel and uploads three artifacts.

## Installation

Copy the bundle from **your platform’s release folder** into the host OFX plug-ins directory, then restart DaVinci Resolve.

If the plug-in does not appear after an upgrade, quit Resolve and delete its OFX plug-in cache file (see paths below), then relaunch.

### macOS

Use the bundle inside **`release/LSP_Simple_Open_DRT_<version>_macos/`**. Copy it to:

- `/Library/OFX/Plugins/` (all users), or
- `~/Library/OFX/Plugins/` (current user)

**Resolve OFX cache (delete if needed):**  
`~/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCacheV2.xml`

### Windows

Use the bundle inside **`release/LSP_Simple_Open_DRT_<version>_windows/`**. Copy it to:

`C:\Program Files\Common Files\OFX\Plugins\`

(Elevation required when writing under `Program Files`.)

**Resolve OFX cache (delete if needed):**  
`%APPDATA%\Blackmagic Design\DaVinci Resolve\Support\OFXPluginCacheV2.xml`

### Linux

Use the bundle inside **`release/LSP_Simple_Open_DRT_<version>_linux/`**. Copy it to:

`/usr/OFX/Plugins/`

## macOS Gatekeeper (unsigned builds)

Release builds are **not signed or notarized**. After you copy the bundle into an OFX folder, macOS may block it from loading in Resolve.

### Method 1 — Terminal (recommended)

Use the path where you actually installed the bundle. Example for the system folder:

```bash
BUNDLE="/Library/OFX/Plugins/LSP_Simple_Open_DRT_<version>.ofx.bundle"

sudo chmod -R 755 "$BUNDLE"
sudo chown -R root:wheel "$BUNDLE"
sudo xattr -dr com.apple.quarantine "$BUNDLE"
sudo codesign --force --deep --sign - "$BUNDLE"
```

For a **user-only** install (`~/Library/OFX/Plugins/...`), use that path in `BUNDLE` and **skip** the `chown root:wheel` line.

When `sudo` asks for your password, type it and press **Enter** (nothing appears on screen — that is normal).

Quit Resolve completely, then reopen it.

### Method 2 — System Settings (no Terminal)

1. Copy a fresh bundle into `/Library/OFX/Plugins/` (or `~/Library/OFX/Plugins/`).
2. Launch Resolve. If macOS shows a security warning, click **Done**.
3. Open **System Settings → Privacy & Security**, scroll down, and click **Allow Anyway** next to the blocked plug-in.
4. In Resolve: **DaVinci Resolve → Preferences → Video Plugins**, find **LSP - Simple Open DRT**, enable it, save, and quit Resolve.
5. Launch Resolve again. When prompted, click **Open Anyway** and enter your Mac password.

### Notes

- If your Mac account has **no login password**, the **Open Anyway** step may not work reliably — use Method 1 instead.
- If still missing in Resolve, delete the OFX cache (path in [Installation](#installation) above) and relaunch.

## License

This project is based on OpenDRT and distributed under **GNU GPL v3**.
