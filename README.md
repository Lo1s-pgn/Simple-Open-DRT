# Simple Open DRT (OFX)

This is a port into an OFX plugin of [OpenDRT by Jed Smith](https://github.com/jedypod/open-display-transform), with a simplified UI, combined preset workflow, cross-platform GPU/CPU render paths and a refactored codebase.

Current version: `1.1.7`

Per-release notes: [CHANGELOG.md](CHANGELOG.md).

Core plugin layout overview: [source/plugin/core/README.md](source/plugin/core/README.md).

## Platform

- Windows (x86_64): CUDA + OpenCL + CPU
- macOS (arm64 + x86_64): Metal + CPU
- Linux (x86_64): CUDA + OpenCL + CPU fallback

## Build

### macOS

From repository root:

```bash
make
```

Manual CMake/Ninja equivalent:

```bash
cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0
cmake --build build --config Release
```

### Linux

From repository root:

```bash
cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

If you want CPU + OpenCL only (no CUDA):

```bash
cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSIMPLE_OPENDRT_LINUX_CUDA=OFF
cmake --build build --config Release
```

### Windows

Recommended helper script:

```bat
source\build_vc.bat
```

Manual CMake/Ninja equivalent (Developer Command Prompt):

```bat
cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Output bundle

- `release/LSP_Simple_Open_DRT_<version>.ofx.bundle`

## Release Archive Workflow

From repository root:

```bash
./archive_version.sh
make clean && make
```

This updates `VERSION`, syncs `source/CMakeLists.txt`, `OpenDRTConstants.h`, `OpenDRTEffect.cpp` (plugin display string), `OpenDRTPresets.h` (`kOpenDRTPortVersion`), root `README.md` (`Current version`), and rebuilds the release bundle.

## Installation

Copy `LSP_Simple_Open_DRT_<version>.ofx.bundle` to the OFX plugin directory:

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


