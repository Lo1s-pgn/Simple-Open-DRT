# OpenFX SDK (vendored)

Vendored copy of the [OpenFX SDK](https://github.com/AcademySoftwareFoundation/openfx) used to build **LSP - Simple Open DRT**.

The plugin build compiles the eight `ofxs*.cpp` support sources from `Support/Library/` into the plugin target. Headers live under `include/` and `Support/include/`.

Override the SDK path at configure time with `-DOFX_SDK_PATH=...` if needed.
