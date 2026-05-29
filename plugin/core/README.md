# `plugin/core`

Flat folder of shared C++/OFX code for **LSP - Simple Open DRT**. GPU backends are **not** here: `**../metal`**, `**../cuda**`, `**../opencl**`; `**OpenDRTProcessor.h**` picks the path and passes `**OpenDRTParams**` (from `**OpenDRTParams.h**`, also included by Metal/CUDA).

## Flow

- `**OpenDRT.cpp**` — `OFX::Plugin::getPluginIDs` → `**openDRTRegisterFactories**`.
- `**OpenDRTEffect.cpp**` — factory + includes; `**OpenDRTEffectBody.inl**` holds `**OpenDRTEffect**` implementation (render, `changedParam`, presets, staging).
- `**OpenDRTDescribe.***` — static descriptor (pages, params, host Metal/CUDA advertisement uses `**OpenDRTRuntimeEnv**`).

## Finder

- **Preset / resolve math:** `**OpenDRTPresets.h`**, `**OpenDRTCPUCore.h**`, `**OpenDRTProcessor.h**` (orchestration + backends).
- **User preset XML:** `**OpenDRTInterop.*`**, paths/dialogs `**OpenDRTPlatform.***`.
- **Look capture/apply helpers:** `**OpenDRTLookSections.*`**  
- **Misc .cpp modules:** preset menu indices (`**OpenDRTPresetMenus.*`**), row layout probe (`**OpenDRTRender.***`), param name classification (`**OpenDRTParamRouter.cpp**` — prototypes forward-declared in `**OpenDRTEffect.cpp**`), tooltips (`**OpenDRTUiState.cpp**`).
- **Logging macros:** `**OpenDRTLog.h`**. Plugin id/version: `**OpenDRTConstants.h**`.

Build and bundle paths: repo root `**README.md**`.