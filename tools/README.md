# Build helpers

Optional scripts to configure and build the plugin with CMake. Each writes a versioned, platform-tagged folder under `release/`.

| Script | Platform | Output folder |
|--------|----------|---------------|
| `macos/opendrt_build.sh` | macOS | `release/LSP_Simple_Open_DRT_<version>_macos/` |
| `windows/opendrt_build.bat` | Windows | `release/LSP_Simple_Open_DRT_<version>_windows/` |
| `linux/opendrt_build.sh` | Linux | `release/LSP_Simple_Open_DRT_<version>_linux/` |

Pass extra CMake configure flags after the script name on macOS/Linux (e.g. `-DOPENDRT_OFX_FAT_ARCHS=OFF`).

**Windows:** requires Visual Studio with MSVC, Ninja, and the CUDA Toolkit (CUDA + OpenCL).

**Linux:** requires Ninja, OpenCL ICD dev headers (`ocl-icd-opencl-dev`), and optionally CUDA for the default build.
