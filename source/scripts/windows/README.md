# `scripts/windows/` — Windows helpers

**`build_vc.bat`** and **`configure_vc.bat`** embed Visual Studio and CUDA paths — adjust them for your machine before use.

Scripts locate the **repository root** from `%~dp0..\..\..` so you can launch them from Explorer without `cd`-ing into `source` first.

From the **repository root**:

- **Build**: `source\scripts\windows\build_vc.bat`
- **Configure** (alternate Ninja tree): `source\scripts\windows\configure_vc.bat`
- **Sign release**: `source\scripts\windows\sign_release.bat` → runs `source\packaging\windows\sign_release.ps1`

See [source README](../../README.md) and the [repository README](../../../README.md).
