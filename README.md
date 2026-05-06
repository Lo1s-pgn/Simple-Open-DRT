# Simple Open DRT (OFX)

**Simple Open DRT** is a DaVinci Resolve OFX plugin based on OpenDRT by Jed Smith, with a simplified UI, combined preset workflow, and cross-platform GPU/CPU render paths.

Current version: `1.1.1`

## Upstream and License

This project is based on OpenDRT and distributed under **GNU GPL v3**.  
Upstream: [open-display-transform](https://github.com/jedypod/open-display-transform)

## Platform Status

- Windows (x86_64): CUDA + OpenCL + CPU
- macOS (arm64 + x86_64): Metal + CPU
- Linux (x86_64): CUDA + OpenCL + CPU fallback

## Build (macOS)

From `source`:

```bash
cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0
cmake --build build --config Release
```

Bundle output:
- `release/LSP_Simple_Open_DRT_<version>.ofx.bundle`

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

