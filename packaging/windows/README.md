# Windows Packaging

This folder contains Windows release packaging assets:

- `LSP_Simple_Open_DRT.iss` - Inno Setup installer definition.
- `sign_release.ps1` - code-signing helper for the built OFX binary and installer.

## Build + Package flow

1. Build the plugin in Release mode:
   - Run **`tools\windows\opendrt_build.bat`** (from repo root or anywhere; it resolves the repo root).
2. Sync **`PluginVersion`** in `LSP_Simple_Open_DRT.iss` with the first line of root **`VERSION`** if they differ.
3. Build installer with Inno Setup:
   - Open `LSP_Simple_Open_DRT.iss` in Inno Setup Compiler and compile.
4. (Optional) Sign artifacts — run **`sign_release.ps1`** in this folder.

The installer reads the bundle from:

- `release/LSP_Simple_Open_DRT_<version>_windows/LSP_Simple_Open_DRT_<version>.ofx.bundle/`

The installer installs into:

- `C:\Program Files\Common Files\OFX\Plugins\`
