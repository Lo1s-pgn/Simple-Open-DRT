# Windows Packaging

This folder contains Windows release packaging assets:

- `LSP_Simple_Open_DRT.iss` - Inno Setup installer definition.
- `sign_release.ps1` - code-signing helper for the built OFX binary and installer.

## Build + Package flow

1. Build the plugin in Release mode:
   - Run **`source\scripts\windows\build_vc.bat`** (from anywhere; it resolves the repo root).
2. Build installer with Inno Setup:
   - Open `LSP_Simple_Open_DRT.iss` in Inno Setup Compiler and compile.
3. (Optional) Sign artifacts — run **`source\scripts\windows\sign_release.bat`** (launches **`sign_release.ps1`** in this folder).

The installer installs into:

- `C:\Program Files\Common Files\OFX\Plugins\`
