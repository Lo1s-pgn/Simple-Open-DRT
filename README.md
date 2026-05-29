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

Copy the bundle from **your platform’s release folder** into the OFX plugin directory:

- Windows: `C:\Program Files\Common Files\OFX\Plugins\`
- macOS: `/Library/OFX/Plugins/`
- Linux: `/usr/OFX/Plugins/`

Then restart Resolve.

## macOS Gatekeeper

If the bundle is unsigned, run:

```bash
sudo chmod -R 755 /Library/OFX/Plugins/LSP_Simple_Open_DRT_<version>.ofx.bundle
sudo chown -R root:wheel /Library/OFX/Plugins/LSP_Simple_Open_DRT_<version>.ofx.bundle
sudo xattr -dr com.apple.quarantine /Library/OFX/Plugins/LSP_Simple_Open_DRT_<version>.ofx.bundle
sudo codesign --force --deep --sign - /Library/OFX/Plugins/LSP_Simple_Open_DRT_<version>.ofx.bundle
```

## License

This project is based on OpenDRT and distributed under **GNU GPL v3**.
