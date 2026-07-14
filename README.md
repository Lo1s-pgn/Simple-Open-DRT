# Simple OpenDRT (OFX)

This is a port into an OFX plugin of [OpenDRT by Jed Smith](https://github.com/jedypod/open-display-transform), with a simplified UI, combined preset workflow, cross-platform GPU/CPU render paths and a refactored codebase.

Current OpenDRT version: `1.1.0`

## Platform

| OS | GPU path |
|----|----------|
| **macOS** (arm64 + x86_64) | Metal + CPU |
| **Windows** (x86_64) | CUDA + OpenCL + CPU |
| **Linux** (x86_64) | CUDA + OpenCL + CPU fallback |

Build on each platform separately. Each build writes a versioned release folder:

| Platform | Release folder |
|----------|----------------|
| macOS | `release/LSP_Simple_Open_DRT_<version>_macos/` |
| Windows | `release/LSP_Simple_Open_DRT_<version>_windows/` |
| Linux | `release/LSP_Simple_Open_DRT_<version>_linux/` |

## Build

**CMake** is the only build system. See [tools/README.md](tools/README.md) for helper scripts.

### Prerequisites

- **CMake** 3.22+
- **macOS:** Xcode Command Line Tools
- **Windows:** Visual Studio 2022 (MSVC), **Ninja**, **CUDA Toolkit**
- **Linux:** **Ninja**, OpenCL ICD dev headers (`ocl-icd-opencl-dev`); CUDA Toolkit optional (enabled by default)

### macOS

```bash
./tools/macos/opendrt_build.sh
```

Or manually:

```bash
cmake -S . -B build/macos -DCMAKE_BUILD_TYPE=Release
cmake --build build/macos --target opendrt_all --parallel
```

Configure options: `-DOPENDRT_OFX_FAT_ARCHS=OFF` (single-arch), `-DOFX_SDK_PATH=...`

### Windows

```powershell
tools\windows\opendrt_build.bat
```

Or manually (Developer Command Prompt or after `vcvars64.bat`):

```powershell
cmake -S . -B build/windows -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl
cmake --build build/windows --target opendrt_all --parallel
```

Optional Windows installer: see [packaging/windows/README.md](packaging/windows/README.md).

### Linux

```bash
./tools/linux/opendrt_build.sh
```

Or manually:

```bash
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux --target opendrt_all --parallel
```

CPU + OpenCL only (no CUDA): add `-DSIMPLE_OPENDRT_LINUX_CUDA=OFF` to the `cmake -S` line.

## GitHub Actions

**Build OFX release** (`.github/workflows/build-ofx-release.yml`) — trigger manually from GitHub → Actions → Run workflow. Uploads macOS, Windows, and Linux artifacts.

## Installation

Copy the bundle from your platform’s release folder into the host OFX plug-ins directory, then restart DaVinci Resolve.

| Platform | OFX folder |
|----------|------------|
| macOS (all users) | `/Library/OFX/Plugins/` |
| macOS (current user) | `~/Library/OFX/Plugins/` |
| Windows | `C:\Program Files\Common Files\OFX\Plugins\` |
| Linux | `/usr/OFX/Plugins/` |

**Resolve OFX cache** (delete if the plug-in does not appear after upgrade):
- macOS: `~/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCacheV2.xml`
- Windows: `%APPDATA%\Blackmagic Design\DaVinci Resolve\Support\OFXPluginCacheV2.xml`

## macOS Gatekeeper (unsigned builds)

```bash
BUNDLE="/Library/OFX/Plugins/LSP_Simple_Open_DRT_<version>.ofx.bundle"

sudo chmod -R 755 "$BUNDLE"
sudo chown -R root:wheel "$BUNDLE"   # skip for ~/Library/OFX/Plugins/
sudo xattr -dr com.apple.quarantine "$BUNDLE"
sudo codesign --force --deep --sign - "$BUNDLE"
```

Quit and relaunch Resolve.

## License

This project is based on OpenDRT and distributed under **GNU GPL v3**.
