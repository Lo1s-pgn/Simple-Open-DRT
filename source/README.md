# `source/` — build tree

This directory is the **CMake project root** (`cmake -S source -B …`). The Makefile at the repo root configures and builds from here.

**`CMakeLists.txt` stays here on purpose:** CMake requires a **`CMakeLists.txt` at the top of `-S source`**. It is not the same kind of “extra” tooling as scripts—moving it under a subfolder would force `cmake -S source/something_else` everywhere. Optional cleanup is `include(...)` fragments under **`cmake/`** (next to `Info.plist.in`), not relocating the root file.

## Layout

| Path | Role |
|------|------|
| **`CMakeLists.txt`** | Plugin target, sources, Metal air/metallib steps, optional CUDA/OpenCL, bundle install into `../release/`. |
| **`cmake/`** | `Info.plist.in`, `plugin_icon.rc.in` and other generator inputs for the OFX bundle. |
| **`plugin/core/`** | Shared C++/OFX logic (effect, params, presets, CPU path). See [plugin/core/README.md](plugin/core/README.md). |
| **`plugin/metal/`** | Metal shader (`.metal`) and ObjC++ bridge (`OpenDRTMetal.mm`). |
| **`plugin/cuda/`** | CUDA kernel (`OpenDRT.cu`) when the platform build enables it. |
| **`plugin/opencl/`** | OpenCL kernel (`.cl`) and script to embed source into a generated header on Windows/Linux. |
| **`packaging/windows/`** | Inno Setup script, signing helper, and Windows packaging notes. |
| **`scripts/windows/`** | Windows `.bat` build/configure/sign helpers — [scripts/windows/README.md](scripts/windows/README.md). |
| **`ofx-sdk/`** | Vendored OpenFX SDK (headers + `Support` library sources compiled into the plugin). |

## Outputs

Intermediate build products live in **`build/`** at the repo root (created by CMake, not checked in). The linked OFX bundle is written to **`release/LSP_Simple_Open_DRT_<version>.ofx.bundle`** (see repo [README.md](../README.md)).
