class OpenDRTEffect : public OFX::ImageEffect {
 public:
  // ===== Effect Lifecycle: construct + initial menu/state sync =====
  explicit OpenDRTEffect(OfxImageEffectHandle handle)
      : ImageEffect(handle) {
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    suppressParamChanged_ = true;
    syncPresetMenus(0.0, getChoice("lookPreset", 0.0, 0), getChoice("tonescalePreset", 0.0, 0));
    suppressParamChanged_ = false;
    updateToggleVisibility(0.0);

    {
      const std::string versionStr = kPluginVersionString;
      OFX::ImageEffectHostDescription* host = OFX::getImageEffectHostDescription();
      const std::string hostName = host ? host->hostName : "unknown";
      const std::string hostLabel = host ? host->hostLabel : "";
      std::string hostVersion;
      if (host) {
        if (!host->versionLabel.empty())
          hostVersion = host->versionLabel;
        else if (host->versionMajor != 0 || host->versionMinor != 0 || host->versionMicro != 0)
          hostVersion = std::to_string(host->versionMajor) + "." + std::to_string(host->versionMinor) + "." +
                        std::to_string(host->versionMicro);
      }
      const std::string buildInfo = __DATE__;
      const std::string bundlePath = LSPOpenDRTLog::getPluginBundleRootPath();
      LSP_OPENDRT_LOG_SESSION_START(kPluginName, versionStr, hostName, hostLabel, hostVersion, buildInfo, bundlePath, "");
    }
  }

  ~OpenDRTEffect() override {
    allowUiParamWrites_ = false;
#if defined(OFX_SUPPORTS_CUDARENDER)
    if (stageSrcPinned_ != nullptr) {
      cudaFreeHost(stageSrcPinned_);
      stageSrcPinned_ = nullptr;
    }
    if (stageDstPinned_ != nullptr) {
      cudaFreeHost(stageDstPinned_);
      stageDstPinned_ = nullptr;
    }
    stagePinnedCapacityFloats_ = 0;
#endif
  }

  // ===== Render Path Entry =====
  // Main render callback.
  // Rule: keep preset/file management out of this path for predictable playback.
  // Render stage map: (1) validate clips/layout, (2) resolve params, (3) pick backend.
void render(const OFX::RenderArguments& args) override {
    RenderUiFence renderUiFenceGuard(*this);
    const auto tRenderStart = std::chrono::steady_clock::now();
    // Never call updateToggleVisibility/setParamEnabled from render: Resolve may render on worker threads
    // while changedParam runs concurrently; hostRenderDepth_ blocks host UI mutations for all threads.
    std::unique_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));
    std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));

    if (!src || !dst) {
      LSP_OPENDRT_LOG_ERROR("render_fetch_image_null");
      OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    if (src->getPixelDepth() != OFX::eBitDepthFloat || dst->getPixelDepth() != OFX::eBitDepthFloat ||
        src->getPixelComponents() != OFX::ePixelComponentRGBA || dst->getPixelComponents() != OFX::ePixelComponentRGBA) {
      LSP_OPENDRT_LOG_ERROR("render_requires_float_rgba");
      OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }

    const OfxRectI bounds = dst->getBounds();
    const int width = bounds.x2 - bounds.x1;
    const int height = bounds.y2 - bounds.y1;
    if (width <= 0 || height <= 0) {
      return;
    }

    const size_t rowBytes = static_cast<size_t>(width) * 4u * sizeof(float);
    const OpenDRTRowLayout srcLayout = detectOpenDRTRowLayout(src.get(), bounds, height, rowBytes);
    const OpenDRTRowLayout dstLayout = detectOpenDRTRowLayout(dst.get(), bounds, height, rowBytes);

    const auto tResolveStart = std::chrono::steady_clock::now();
    OpenDRTRawValues raw = readRawValues(args.time);
    OpenDRTParams params = resolveParams(raw);
    OpenDRTRuntime::perfLog("Param resolve", tResolveStart);

    if (!processor_) {
      processor_ = std::make_unique<OpenDRTProcessor>(params);
    } else {
      processor_->setParams(params);
    }

#if defined(OFX_SUPPORTS_CUDARENDER)
    // Optional OFX host CUDA mode:
    // - Controlled by OpenDRTRuntime::selectedCudaRenderMode().
    // - Uses host-provided CUDA stream and device pointers from fetchImage().
    // - Avoids host<->device staging copies.
    // Note-to-self:
    // This is the fastest route for playback. If I see "Backend render direct"
    // in logs on a CUDA-enabled host, this branch was not taken.
    const bool preferHostCuda = (OpenDRTRuntime::selectedCudaRenderMode() == OpenDRTRuntime::CudaRenderMode::HostPreferred);
    const bool tryHostCuda = preferHostCuda && args.isEnabledCudaRender && (args.pCudaStream != nullptr);
    if (tryHostCuda) {
      const auto tHostCuda = std::chrono::steady_clock::now();
      const float* srcDevice = static_cast<const float*>(src->getPixelData());
      float* dstDevice = static_cast<float*>(dst->getPixelData());
      const int srcRb = src->getRowBytes();
      const int dstRb = dst->getRowBytes();
      const size_t srcRowBytes = srcRb < 0 ? static_cast<size_t>(-srcRb) : static_cast<size_t>(srcRb);
      const size_t dstRowBytes = dstRb < 0 ? static_cast<size_t>(-dstRb) : static_cast<size_t>(dstRb);
      if (srcDevice != nullptr && dstDevice != nullptr &&
          processor_->renderCUDAHostBuffers(srcDevice, dstDevice, width, height, srcRowBytes, dstRowBytes, args.pCudaStream)) {
        OpenDRTRuntime::perfLog("Backend render host CUDA", tHostCuda);
        OpenDRTRuntime::perfLog("Render total", tRenderStart);
        return;
      }
      if (OpenDRTRuntime::debugLogEnabled()) {
        std::fprintf(stderr, "[Simple_Open_DRT] Host CUDA render failed.\n");
      }
      // When the host explicitly provided CUDA memory, do not fall through into CPU staging
      // paths that assume host-readable pointers.
      OFX::throwSuiteStatusException(kOfxStatFailed);
    }
#endif

#if defined(__APPLE__)
    // Host Metal mode (macOS):
    // - Uses host-provided command queue + MTLBuffer image handles.
    // - Avoids plugin-owned CPU staging copies.
    const bool preferHostMetal = (OpenDRTRuntime::selectedMetalRenderMode() == OpenDRTRuntime::MetalRenderMode::HostPreferred);
    const bool tryHostMetal = preferHostMetal && args.isEnabledMetalRender && (args.pMetalCmdQ != nullptr);
    if (tryHostMetal) {
      const auto tHostMetal = std::chrono::steady_clock::now();
      const void* srcMetalBuffer = src->getPixelData();
      void* dstMetalBuffer = dst->getPixelData();
      const int srcRb = src->getRowBytes();
      const int dstRb = dst->getRowBytes();
      const size_t srcRowBytes = srcRb < 0 ? static_cast<size_t>(-srcRb) : static_cast<size_t>(srcRb);
      const size_t dstRowBytes = dstRb < 0 ? static_cast<size_t>(-dstRb) : static_cast<size_t>(dstRb);
      if (srcMetalBuffer != nullptr && dstMetalBuffer != nullptr &&
          processor_->renderMetalHostBuffers(
              srcMetalBuffer,
              dstMetalBuffer,
              width,
              height,
              srcRowBytes,
              dstRowBytes,
              bounds.x1,
              bounds.y1,
              args.pMetalCmdQ)) {
        OpenDRTMetal::resetHostMetalFailureState();
        OpenDRTRuntime::perfLog("Backend render host Metal", tHostMetal);
        OpenDRTRuntime::perfLog("Render total", tRenderStart);
        return;
      }
      if (OpenDRTRuntime::debugLogEnabled()) {
        std::fprintf(stderr, "[Simple_Open_DRT] Host Metal render failed.\n");
      }
      // Safe fallback: continue into existing internal render path.
      // This preserves stability if host-Metal submission fails transiently.
    }
#endif

    bool rendered = false;
    // Fast path: process directly on host image memory layout (no extra staging vectors).
    if (!OpenDRTRuntime::forceStageCopyEnabled() && srcLayout.valid && dstLayout.valid) {
      const auto tBackendDirect = std::chrono::steady_clock::now();
      rendered = processor_->renderWithLayout(
          srcLayout.base, dstLayout.base, width, height, srcLayout.pitchBytes, dstLayout.pitchBytes, true);
      OpenDRTRuntime::perfLog("Backend render direct", tBackendDirect);
    }

    // Fallback path: stable staged copy used for irregular host layouts.
    if (!rendered) {
      const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
      if (!ensureStageBuffers(pixelCount)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
      }
      float* srcStage = stageSrcPtr();
      float* dstStage = stageDstPtr();
      if (!srcStage || !dstStage) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
      }

      const auto tStageCopyStart = std::chrono::steady_clock::now();
      if (srcLayout.valid && srcLayout.contiguous) {
        std::memcpy(srcStage, srcLayout.base, rowBytes * static_cast<size_t>(height));
      } else {
        // Row fallback for hosts with non-contiguous row layout.
        for (int y = bounds.y1; y < bounds.y2; ++y) {
          const int localY = y - bounds.y1;
          float* sp = static_cast<float*>(src->getPixelAddress(bounds.x1, y));
          float* rowDst = srcStage + static_cast<size_t>(localY) * static_cast<size_t>(width) * 4u;
          if (sp != nullptr) {
            std::memcpy(rowDst, sp, rowBytes);
          } else {
            std::memset(rowDst, 0, rowBytes);
          }
        }
      }
      OpenDRTRuntime::perfLog("Host src staging", tStageCopyStart);

      const auto tBackendStart = std::chrono::steady_clock::now();
      rendered = processor_->render(srcStage, dstStage, width, height, true);
      OpenDRTRuntime::perfLog("Backend render staging", tBackendStart);
      if (!rendered) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
      }

      const auto tDstCopyStart = std::chrono::steady_clock::now();
      if (dstLayout.valid && dstLayout.contiguous) {
        std::memcpy(dstLayout.base, dstStage, rowBytes * static_cast<size_t>(height));
      } else {
        for (int y = bounds.y1; y < bounds.y2; ++y) {
          const int localY = y - bounds.y1;
          float* dp = static_cast<float*>(dst->getPixelAddress(bounds.x1, y));
          if (!dp) continue;
          const float* rowSrc = dstStage + static_cast<size_t>(localY) * static_cast<size_t>(width) * 4u;
          std::memcpy(dp, rowSrc, rowBytes);
        }
      }
      OpenDRTRuntime::perfLog("Host dst copy", tDstCopyStart);
    }

    OpenDRTRuntime::perfLog("Render total", tRenderStart);
  }

  void syncPrivateData() override {
    // Resolve can invoke syncPrivateData while a render is in flight (or from a context that races render workers).
    // Never touch OFX param UI (visibility, presetState, labels) until render() has released RenderUiFence.
    if (hostRenderDepth_.load(std::memory_order_acquire) != 0) {
      return;
    }
    if (deferredHostUiFlush_.exchange(false, std::memory_order_acq_rel)) {
      try {
        const double t = timeLineGetTime();
        updateToggleVisibility(t);
        updatePresetStateFromCurrent(t);
      } catch (...) {
      }
    }
  }

  // ===== UI Event Entry =====
  // UI/param callback entry point.
  // Keep this deterministic: mutate params/state, then refresh dependent UI labels/states.
  // UI event router: preset orchestration first, then generic change propagation.
void changedParam(const OFX::InstanceChangedArgs& args, const std::string& paramName) override {
    try {
      if (shouldSkipChangedParam(args, paramName)) return;
      if (handleLookPresetChanged(args, paramName)) return;
      if (handleTonescalePresetChanged(args, paramName)) return;
      if (handleDisplayRouting(args, paramName)) return;
      if (handleResetRouting(args, paramName)) return;
      if (handleSupportAction(paramName)) return;
      if (handleCombinedPresetAction(args, paramName)) return;
      if (handleAdvancedParamRouting(args, paramName)) return;
    } catch (...) {
      // Swallow callback exceptions to avoid host crashes while stabilizing.
    }
  }

  void getClipPreferences(OFX::ClipPreferencesSetter& clipPreferences) override {
    clipPreferences.setClipBitDepth(*dstClip_, OFX::eBitDepthFloat);
    clipPreferences.setClipComponents(*dstClip_, OFX::ePixelComponentRGBA);
  }

 private:
  static constexpr int kBuiltInDisplayPresetCount = 9;

  struct FlagScope {
    explicit FlagScope(bool& f) : flag(f) { flag = true; }
    ~FlagScope() { flag = false; }
    bool& flag;
  };

  bool shouldSkipChangedParam(const OFX::InstanceChangedArgs& args, const std::string& paramName) const {
    if (suppressParamChanged_) return true;
    if (args.reason == OFX::eChangeTime) return true;
    if (args.reason == OFX::eChangePluginEdit) return true;
    if (paramName == "presetState") return true;
    return false;
  }

  bool handleDisplayRouting(const OFX::InstanceChangedArgs& args, const std::string& paramName) {
    if (paramName == "creativeWhitePreset") {
      const int cwpPreset = getChoice("creativeWhitePreset", args.time, 2);
      FlagScope scope(suppressParamChanged_);
      writeCreativeWhitePresetToParams(cwpPreset, *this);
      updatePresetStateFromCurrent(args.time);
      return true;
    }
    if (paramName == "displayEncodingPreset") {
      int preset = getChoice("displayEncodingPreset", args.time, 0);
      FlagScope scope(suppressParamChanged_);
      writeDisplayPresetToParams(preset, *this);
      const int eotf = getChoice("eotf", args.time, 2);
      setDouble("tn_Lp", (eotf == 4 || eotf == 5) ? 1000.0 : 100.0);
      updatePresetStateFromCurrent(args.time);
      return true;
    }
    if (paramName == "eotf") {
      const int eotf = getChoice("eotf", args.time, 2);
      FlagScope scope(suppressParamChanged_);
      setDouble("tn_Lp", (eotf == 4 || eotf == 5) ? 1000.0 : 100.0);
    }
    if (paramName == "display_gamut" || paramName == "eotf" || paramName == "tn_su") {
      const int matchedPreset = matchingDisplayEncodingPresetForCurrent(args.time);
      if (matchedPreset >= 0 && getChoice("displayEncodingPreset", args.time, 0) != matchedPreset) {
        FlagScope scope(suppressParamChanged_);
        setChoice("displayEncodingPreset", matchedPreset);
      }
      updatePresetStateFromCurrent(args.time);
      return true;
    }
    return false;
  }

  bool handleResetRouting(const OFX::InstanceChangedArgs& args, const std::string& paramName) {
    if (paramName != "reset_tonescale" &&
        paramName != "reset_render_space" &&
        paramName != "reset_mid_purity" &&
        paramName != "reset_purity_compression" &&
        paramName != "reset_brilliance" &&
        paramName != "reset_hue" &&
        paramName != "reset_white_point" &&
        paramName != "reset_look_settings") {
      return false;
    }
    OpenDRTParams expected{};
    if (!buildPresetBaseline(args.time, &expected)) return true;
    FlagScope scope(suppressParamChanged_);
    if (paramName == "reset_look_settings") {
      constexpr int kDefaultLookPreset = 0;
      constexpr int kDefaultTonescalePreset = 1;
      setChoice("lookPreset", kDefaultLookPreset);
      setChoice("tonescalePreset", kDefaultTonescalePreset);
      writePresetToParams(kDefaultLookPreset, *this);
      writeTonescalePresetToParams(kDefaultTonescalePreset, *this);
      setChoice("creativeWhitePreset", std::max(0, std::min(5, getInt("cwp", args.time, 2))));
      setBool("tn_enable", true);
      setBool("rs_enable", true);
      setBool("wp_enable", true);
    } else if (paramName == "reset_tonescale") {
      setBool("tn_enable", true);
      applyTonescaleFromBaseline(expected);
    } else if (paramName == "reset_render_space") {
      setBool("rs_enable", true);
      applyRenderSpaceFromBaseline(expected);
    } else if (paramName == "reset_mid_purity") {
      applyMidPurityFromBaseline(expected);
    } else if (paramName == "reset_purity_compression") {
      applyPurityCompressionFromBaseline(expected);
    } else if (paramName == "reset_brilliance") {
      applyBrillianceFromBaseline(expected);
    } else if (paramName == "reset_hue") {
      applyHueFromBaseline(expected);
    } else if (paramName == "reset_white_point") {
      setBool("wp_enable", true);
      OpenDRTLookSections::applyWhitePoint(*this, expected);
    }
    updateToggleVisibility(args.time);
    updatePresetStateFromCurrent(args.time);
    return true;
  }

  bool handleSupportAction(const std::string& paramName) {
    if (paramName == "supportHelp") {
      (void)openExternalUrl("https://github.com/Lo1s-pgn/Simple-Open-DRT#readme");
      return true;
    }
    if (paramName == "supportOpenLog") {
      LSPOpenDRTLog::ensureLogFileExists();
      const std::string logPath = LSPOpenDRTLog::getLogPath();
      if (!openExternalUrl(logPath)) LSP_OPENDRT_LOG_ERROR("open_log_failed");
      return true;
    }
    if (paramName == "supportReportIssue") {
      (void)openExternalUrl("https://github.com/Lo1s-pgn/Simple-Open-DRT/issues");
      return true;
    }
    if (paramName == "supportOpenDrtRepo") {
      (void)openExternalUrl("https://github.com/jedypod/open-display-transform");
      return true;
    }
    return false;
  }

  bool handleCombinedPresetAction(const OFX::InstanceChangedArgs& args, const std::string& paramName) {
    if (paramName == "userPresetCombined") {
      const int idx = getChoice("userPresetCombined", args.time, 0);
      if (idx <= 0) return true;
      const std::vector<std::string> names = combinedPresetXmlNames();
      const int rel = idx - 1;
      if (rel < 0 || rel >= static_cast<int>(names.size())) return true;
      const std::filesystem::path path = combinedUserPresetDirPath() / (names[static_cast<size_t>(rel)] + ".xml");
      std::string presetName;
      LookPresetValues look{};
      TonescalePresetValues tone{};
      if (!readCombinedPresetXmlFile(path, &presetName, &look, &tone)) return true;
      FlagScope scope(suppressParamChanged_);
      writeLookValuesToParams(look, *this);
      writeTonescaleValuesToParams(tone, *this);
      updateToggleVisibility(args.time);
      updatePresetStateFromCurrent(args.time);
      return true;
    }
    if (paramName == "userPresetExportXml") {
      std::string defaultName = "Simple_Open_DRT_Preset.xml";
      const int idx = getChoice("userPresetCombined", args.time, 0);
      if (idx > 0) {
        const std::vector<std::string> names = combinedPresetXmlNames();
        const int rel = idx - 1;
        if (rel >= 0 && rel < static_cast<int>(names.size())) defaultName = names[static_cast<size_t>(rel)] + ".xml";
      }
      std::string pathStr = pickSaveXmlFilePath(defaultName);
      if (pathStr.empty()) return true;
      std::filesystem::path path(pathStr);
      if (path.extension() != ".xml") path += ".xml";
      const std::string name = combinedPresetFileStemFromPath(path.string());
      if (std::filesystem::exists(path) && !confirmOverwriteDialog(name)) return true;
      const LookPresetValues look = captureCurrentLookValues(args.time);
      const TonescalePresetValues tone = captureCurrentTonescaleValues(args.time);
      if (!writeCombinedPresetXmlFile(path, name, look, tone)) return true;
      syncCombinedPresetMenuFromDisk(args.time, 0);
      const std::vector<std::string> names = combinedPresetXmlNames();
      for (size_t i = 0; i < names.size(); ++i) {
        if (names[i] == name) { setChoice("userPresetCombined", static_cast<int>(i + 1)); break; }
      }
      return true;
    }
    if (paramName == "userPresetImportXml") {
      const std::string srcPath = pickOpenXmlFilePath();
      if (srcPath.empty()) return true;
      std::string name;
      LookPresetValues look{};
      TonescalePresetValues tone{};
      if (!readCombinedPresetXmlFile(srcPath, &name, &look, &tone)) {
        showInfoDialog("Invalid XML preset file.");
        return true;
      }
      const std::filesystem::path dst = combinedUserPresetDirPath() / (name + ".xml");
      if (std::filesystem::exists(dst) && !confirmOverwriteDialog(name)) return true;
      if (!writeCombinedPresetXmlFile(dst, name, look, tone)) return true;
      FlagScope scope(suppressParamChanged_);
      writeLookValuesToParams(look, *this);
      writeTonescaleValuesToParams(tone, *this);
      syncCombinedPresetMenuFromDisk(args.time, 0);
      const std::vector<std::string> names = combinedPresetXmlNames();
      for (size_t i = 0; i < names.size(); ++i) {
        if (names[i] == name) { setChoice("userPresetCombined", static_cast<int>(i + 1)); break; }
      }
      updateToggleVisibility(args.time);
      updatePresetStateFromCurrent(args.time);
      return true;
    }
    if (paramName == "userPresetRefreshXml") {
      syncCombinedPresetMenuFromDisk(args.time, getChoice("userPresetCombined", args.time, 0));
      return true;
    }
    return false;
  }

  bool handleAdvancedParamRouting(const OFX::InstanceChangedArgs& args, const std::string& paramName) {
    if (!isAdvancedParam(paramName)) return false;
    FlagScope scope(suppressParamChanged_);
    if (paramName == "wp_enable" && !getBool("wp_enable", args.time, 1)) {
      OpenDRTParams expected{};
      if (buildPresetBaseline(args.time, &expected)) {
        OpenDRTLookSections::applyWhitePoint(*this, expected);
      }
    }
    if (paramName == "cwp" || paramName == "cwp_lm") {
      const int cwp = std::max(0, std::min(5, getInt("cwp", args.time, 2)));
      setChoice("creativeWhitePreset", cwp);
    }
    if (isVisibilityToggleParam(paramName)) {
      updateToggleVisibility(args.time);
    }
    updatePresetStateFromCurrent(args.time);
    return true;
  }

  bool handleLookPresetChanged(const OFX::InstanceChangedArgs& args, const std::string& paramName) {
    if (paramName != "lookPreset") return false;
    int look = getChoice("lookPreset", args.time, 0);
    FlagScope scope(suppressParamChanged_);
    if (isCustomLookPresetIndex(look)) {
      updateToggleVisibility(args.time);
      updatePresetStateFromCurrent(args.time);
      return true;
    }
    writePresetToParams(look, *this);
    const TonescalePresetValues tone = captureCurrentTonescaleValues(args.time);
    int matchedTone = 1;
    for (size_t i = 0; i < kTonescalePresets.size(); ++i) {
      const TonescalePresetValues& p = kTonescalePresets[i];
      const bool match =
          std::fabs(tone.tn_con - p.tn_con) <= 1e-6f &&
          std::fabs(tone.tn_sh - p.tn_sh) <= 1e-6f &&
          std::fabs(tone.tn_toe - p.tn_toe) <= 1e-6f &&
          std::fabs(tone.tn_off - p.tn_off) <= 1e-6f &&
          tone.tn_hcon_enable == p.tn_hcon_enable &&
          std::fabs(tone.tn_hcon - p.tn_hcon) <= 1e-6f &&
          std::fabs(tone.tn_hcon_pv - p.tn_hcon_pv) <= 1e-6f &&
          std::fabs(tone.tn_hcon_st - p.tn_hcon_st) <= 1e-6f &&
          tone.tn_lcon_enable == p.tn_lcon_enable &&
          std::fabs(tone.tn_lcon - p.tn_lcon) <= 1e-6f &&
          std::fabs(tone.tn_lcon_w - p.tn_lcon_w) <= 1e-6f;
      if (match) {
        matchedTone = static_cast<int>(i);
        break;
      }
    }
    setChoice("tonescalePreset", matchedTone);
    setChoice("creativeWhitePreset", std::max(0, std::min(5, getInt("cwp", args.time, 2))));
    updateToggleVisibility(args.time);
    updatePresetStateFromCurrent(args.time);
    return true;
  }

  bool handleTonescalePresetChanged(const OFX::InstanceChangedArgs& args, const std::string& paramName) {
    if (paramName != "tonescalePreset") return false;
    const int tsPreset = getChoice("tonescalePreset", args.time, 0);
    FlagScope scope(suppressParamChanged_);
    if (isCustomTonescalePresetIndex(tsPreset)) {
      updateToggleVisibility(args.time);
      updatePresetStateFromCurrent(args.time);
      return true;
    }
    writeTonescalePresetToParams(tsPreset, *this);
    updateToggleVisibility(args.time);
    updatePresetStateFromCurrent(args.time);
    return true;
  }

  /** Raised for the lifetime of render(): forbids host param UI mutations from any thread while a render is in flight. */
  struct RenderUiFence {
    explicit RenderUiFence(OpenDRTEffect& e) : self(e) {
      self.hostRenderDepth_.fetch_add(1, std::memory_order_acq_rel);
    }
    ~RenderUiFence() { self.hostRenderDepth_.fetch_sub(1, std::memory_order_acq_rel); }

   private:
    OpenDRTEffect& self;
  };

  bool uiHostParamWritesSafeNow() const noexcept {
    return allowUiParamWrites_ && (hostRenderDepth_.load(std::memory_order_acquire) == 0);
  }

  // ===== Staging Buffers: host memory used by non-direct render paths =====
  bool ensureStageBuffers(size_t pixelCount) {
#if defined(OFX_SUPPORTS_CUDARENDER)
    // Prefer pinned host buffers for staged path to improve CUDA transfer throughput.
    if (stageSrcPinned_ != nullptr && stageDstPinned_ != nullptr && stagePinnedCapacityFloats_ == pixelCount) return true;
    if (stageSrcPinned_ != nullptr) {
      cudaFreeHost(stageSrcPinned_);
      stageSrcPinned_ = nullptr;
    }
    if (stageDstPinned_ != nullptr) {
      cudaFreeHost(stageDstPinned_);
      stageDstPinned_ = nullptr;
    }
    stagePinnedCapacityFloats_ = 0;
    const size_t bytes = pixelCount * sizeof(float);
    if (cudaHostAlloc(reinterpret_cast<void**>(&stageSrcPinned_), bytes, cudaHostAllocDefault) == cudaSuccess &&
        cudaHostAlloc(reinterpret_cast<void**>(&stageDstPinned_), bytes, cudaHostAllocDefault) == cudaSuccess) {
      stagePinnedCapacityFloats_ = pixelCount;
      return true;
    }
    if (stageSrcPinned_ != nullptr) {
      cudaFreeHost(stageSrcPinned_);
      stageSrcPinned_ = nullptr;
    }
    if (stageDstPinned_ != nullptr) {
      cudaFreeHost(stageDstPinned_);
      stageDstPinned_ = nullptr;
    }
    stagePinnedCapacityFloats_ = 0;
#endif
    if (srcPixels_.size() != pixelCount) srcPixels_.assign(pixelCount, 0.0f);
    if (dstPixels_.size() != pixelCount) dstPixels_.assign(pixelCount, 0.0f);
    return true;
  }

  float* stageSrcPtr() {
#if defined(OFX_SUPPORTS_CUDARENDER)
    if (stageSrcPinned_ != nullptr) return stageSrcPinned_;
#endif
    return srcPixels_.empty() ? nullptr : srcPixels_.data();
  }

  float* stageDstPtr() {
#if defined(OFX_SUPPORTS_CUDARENDER)
    if (stageDstPinned_ != nullptr) return stageDstPinned_;
#endif
    return dstPixels_.empty() ? nullptr : dstPixels_.data();
  }

  // ===== Param Classification: route updates and state recomputation =====
  bool isAdvancedParam(const std::string& name) const {
    return openDrtIsAdvancedParam(name);
  }

  bool isVisibilityToggleParam(const std::string& name) const {
    return openDrtIsVisibilityToggleParam(name);
  }

  bool almostEqual(float a, float b, float eps = 1e-6f) const {
    return std::fabs(a - b) <= eps;
  }

  int matchingDisplayEncodingPresetForCurrent(double time) const {
    const int currentGamut = getChoice("display_gamut", time, 0);
    const int currentEotf = getChoice("eotf", time, 2);
    const int currentSurround = getChoice("tn_su", time, 1);
    for (int preset = 0; preset < kBuiltInDisplayPresetCount; ++preset) {
      OpenDRTParams expected{};
      applyDisplayEncodingPreset(expected, preset);
      if (expected.display_gamut == currentGamut &&
          expected.eotf == currentEotf &&
          expected.tn_su == currentSurround) {
        return preset;
      }
    }
    return -1;
  }

  // ===== Snapshot Capture: current UI values -> preset structs =====
  TonescalePresetValues captureCurrentTonescaleValues(double time) const {
    return OpenDRTLookSections::captureTonescale(*this, time);
  }

  LookPresetValues captureCurrentLookValues(double time) const {
    return OpenDRTLookSections::captureLook(*this, time);
  }

  // ===== Preset Baseline Resolver =====
  // Computes the expected "clean" state for current look/tonescale/display selector choices.
  bool buildPresetBaseline(double time, OpenDRTParams* expected) const {
    if (expected == nullptr) return false;
    const int look = getChoice("lookPreset", time, 0);
    const int tsPreset = getChoice("tonescalePreset", time, 0);
    const int displayPreset = getChoice("displayEncodingPreset", time, 0);
    OpenDRTParams out{};
    // Step 1: Start from built-in look baseline.
    applyLookPresetToResolved(out, look);

    // Step 2: Apply selected built-in tonescale preset.
    applyTonescalePresetToResolved(out, tsPreset);

    // Step 3: Apply display preset defaults.
    applyDisplayEncodingPreset(out, displayPreset);
    out.clamp = 1;
    *expected = out;
    return true;
  }

  // ===== Category Reset Writers: apply selected baseline by section =====
  void applyTonescaleFromBaseline(const OpenDRTParams& p) {
    OpenDRTLookSections::applyTonescale(*this, p);
  }

  void applyRenderSpaceFromBaseline(const OpenDRTParams& p) {
    OpenDRTLookSections::applyRenderSpace(*this, p);
  }

  void applyMidPurityFromBaseline(const OpenDRTParams& p) {
    OpenDRTLookSections::applyMidPurity(*this, p);
  }

  void applyPurityCompressionFromBaseline(const OpenDRTParams& p) {
    OpenDRTLookSections::applyPurityCompression(*this, p);
  }

  void applyBrillianceFromBaseline(const OpenDRTParams& p) {
    OpenDRTLookSections::applyBrilliance(*this, p);
  }

  void applyHueFromBaseline(const OpenDRTParams& p) {
    OpenDRTLookSections::applyHue(*this, p);
  }

  // ===== Dirty-State Evaluation: compare live params against computed baseline =====
  bool isCurrentEqualToPresetBaseline(double time, bool* tonescaleCleanOut = nullptr, bool* creativeWhiteCleanOut = nullptr, bool* displayEncodingCleanOut = nullptr) const {
    OpenDRTParams expected{};
    if (!buildPresetBaseline(time, &expected)) return false;

    // tonescaleClean is split out so we can mark tonescale menu "(Modified)" independently
    // from overall look modified state.
    const bool tonescaleClean =
      almostEqual(getDouble("tn_con", time, expected.tn_con), expected.tn_con) &&
      almostEqual(getDouble("tn_sh", time, expected.tn_sh), expected.tn_sh) &&
      almostEqual(getDouble("tn_toe", time, expected.tn_toe), expected.tn_toe) &&
      almostEqual(getDouble("tn_off", time, expected.tn_off), expected.tn_off) &&
      (getBool("tn_hcon_enable", time, expected.tn_hcon_enable) == expected.tn_hcon_enable) &&
      almostEqual(getDouble("tn_hcon", time, expected.tn_hcon), expected.tn_hcon) &&
      almostEqual(getDouble("tn_hcon_pv", time, expected.tn_hcon_pv), expected.tn_hcon_pv) &&
      almostEqual(getDouble("tn_hcon_st", time, expected.tn_hcon_st), expected.tn_hcon_st) &&
      (getBool("tn_lcon_enable", time, expected.tn_lcon_enable) == expected.tn_lcon_enable) &&
      almostEqual(getDouble("tn_lcon", time, expected.tn_lcon), expected.tn_lcon) &&
      almostEqual(getDouble("tn_lcon_w", time, expected.tn_lcon_w), expected.tn_lcon_w);

    if (tonescaleCleanOut) *tonescaleCleanOut = tonescaleClean;

    const bool creativeWhiteClean =
      (getChoice("creativeWhitePreset", time, expected.cwp) == expected.cwp) &&
      (getInt("cwp", time, expected.cwp) == expected.cwp);

    if (creativeWhiteCleanOut) *creativeWhiteCleanOut = creativeWhiteClean;

    const bool displayEncodingClean =
      (getChoice("display_gamut", time, expected.display_gamut) == expected.display_gamut) &&
      (getChoice("eotf", time, expected.eotf) == expected.eotf) &&
      (getChoice("tn_su", time, expected.tn_su) == expected.tn_su);

    if (displayEncodingCleanOut) *displayEncodingCleanOut = displayEncodingClean;

    // Overall "clean" is look-only: advanced controls + creative white.
    // Display Encoding tracks its own custom/modified state separately.
    const bool clean =
      tonescaleClean &&
      almostEqual(getDouble("rs_sa", time, expected.rs_sa), expected.rs_sa) &&
      almostEqual(getDouble("rs_rw", time, expected.rs_rw), expected.rs_rw) &&
      almostEqual(getDouble("rs_bw", time, expected.rs_bw), expected.rs_bw) &&
      (getBool("pt_enable", time, expected.pt_enable) == expected.pt_enable) &&
      almostEqual(getDouble("pt_lml", time, expected.pt_lml), expected.pt_lml) &&
      almostEqual(getDouble("pt_lml_r", time, expected.pt_lml_r), expected.pt_lml_r) &&
      almostEqual(getDouble("pt_lml_g", time, expected.pt_lml_g), expected.pt_lml_g) &&
      almostEqual(getDouble("pt_lml_b", time, expected.pt_lml_b), expected.pt_lml_b) &&
      almostEqual(getDouble("pt_lmh", time, expected.pt_lmh), expected.pt_lmh) &&
      almostEqual(getDouble("pt_lmh_r", time, expected.pt_lmh_r), expected.pt_lmh_r) &&
      almostEqual(getDouble("pt_lmh_b", time, expected.pt_lmh_b), expected.pt_lmh_b) &&
      (getBool("ptl_enable", time, expected.ptl_enable) == expected.ptl_enable) &&
      almostEqual(getDouble("ptl_c", time, expected.ptl_c), expected.ptl_c) &&
      almostEqual(getDouble("ptl_m", time, expected.ptl_m), expected.ptl_m) &&
      almostEqual(getDouble("ptl_y", time, expected.ptl_y), expected.ptl_y) &&
      (getBool("ptm_enable", time, expected.ptm_enable) == expected.ptm_enable) &&
      almostEqual(getDouble("ptm_low", time, expected.ptm_low), expected.ptm_low) &&
      almostEqual(getDouble("ptm_low_rng", time, expected.ptm_low_rng), expected.ptm_low_rng) &&
      almostEqual(getDouble("ptm_low_st", time, expected.ptm_low_st), expected.ptm_low_st) &&
      almostEqual(getDouble("ptm_high", time, expected.ptm_high), expected.ptm_high) &&
      almostEqual(getDouble("ptm_high_rng", time, expected.ptm_high_rng), expected.ptm_high_rng) &&
      almostEqual(getDouble("ptm_high_st", time, expected.ptm_high_st), expected.ptm_high_st) &&
      (getBool("brl_enable", time, expected.brl_enable) == expected.brl_enable) &&
      almostEqual(getDouble("brl", time, expected.brl), expected.brl) &&
      almostEqual(getDouble("brl_r", time, expected.brl_r), expected.brl_r) &&
      almostEqual(getDouble("brl_g", time, expected.brl_g), expected.brl_g) &&
      almostEqual(getDouble("brl_b", time, expected.brl_b), expected.brl_b) &&
      almostEqual(getDouble("brl_rng", time, expected.brl_rng), expected.brl_rng) &&
      almostEqual(getDouble("brl_st", time, expected.brl_st), expected.brl_st) &&
      (getBool("brlp_enable", time, expected.brlp_enable) == expected.brlp_enable) &&
      almostEqual(getDouble("brlp", time, expected.brlp), expected.brlp) &&
      almostEqual(getDouble("brlp_r", time, expected.brlp_r), expected.brlp_r) &&
      almostEqual(getDouble("brlp_g", time, expected.brlp_g), expected.brlp_g) &&
      almostEqual(getDouble("brlp_b", time, expected.brlp_b), expected.brlp_b) &&
      (getBool("hc_enable", time, expected.hc_enable) == expected.hc_enable) &&
      almostEqual(getDouble("hc_r", time, expected.hc_r), expected.hc_r) &&
      almostEqual(getDouble("hc_r_rng", time, expected.hc_r_rng), expected.hc_r_rng) &&
      (getBool("hs_rgb_enable", time, expected.hs_rgb_enable) == expected.hs_rgb_enable) &&
      almostEqual(getDouble("hs_r", time, expected.hs_r), expected.hs_r) &&
      almostEqual(getDouble("hs_r_rng", time, expected.hs_r_rng), expected.hs_r_rng) &&
      almostEqual(getDouble("hs_g", time, expected.hs_g), expected.hs_g) &&
      almostEqual(getDouble("hs_g_rng", time, expected.hs_g_rng), expected.hs_g_rng) &&
      almostEqual(getDouble("hs_b", time, expected.hs_b), expected.hs_b) &&
      almostEqual(getDouble("hs_b_rng", time, expected.hs_b_rng), expected.hs_b_rng) &&
      (getBool("hs_cmy_enable", time, expected.hs_cmy_enable) == expected.hs_cmy_enable) &&
      almostEqual(getDouble("hs_c", time, expected.hs_c), expected.hs_c) &&
      almostEqual(getDouble("hs_c_rng", time, expected.hs_c_rng), expected.hs_c_rng) &&
      almostEqual(getDouble("hs_m", time, expected.hs_m), expected.hs_m) &&
      almostEqual(getDouble("hs_m_rng", time, expected.hs_m_rng), expected.hs_m_rng) &&
      almostEqual(getDouble("hs_y", time, expected.hs_y), expected.hs_y) &&
      almostEqual(getDouble("hs_y_rng", time, expected.hs_y_rng), expected.hs_y_rng) &&
      creativeWhiteClean &&
      almostEqual(getDouble("cwp_lm", time, expected.cwp_lm), expected.cwp_lm);

    return clean;
  }

  void updatePresetStateFromCurrent(double time) {
    if (!uiHostParamWritesSafeNow()) {
      deferredHostUiFlush_.store(true, std::memory_order_release);
      return;
    }
    bool tonescaleClean = true;
    bool creativeWhiteClean = true;
    bool displayEncodingClean = true;
    const bool clean = isCurrentEqualToPresetBaseline(time, &tonescaleClean, &creativeWhiteClean, &displayEncodingClean);
    // presetState drives UI readout and Discard availability.
    setInt("presetState", clean ? 0 : 1);
    // As soon as values diverge, move selectors to explicit "(custom)" entries.
    // This keeps modified values detached from the originating preset labels.
    const int lookIdx = getChoice("lookPreset", time, 0);
    const int toneIdx = getChoice("tonescalePreset", time, 0);
    const int customLookIdx = customLookPresetIndex();
    const int customToneIdx = customTonescalePresetIndex();
    if (!clean && !isCustomLookPresetIndex(lookIdx)) setChoice("lookPreset", customLookIdx);
    if (!tonescaleClean && !isCustomTonescalePresetIndex(toneIdx)) setChoice("tonescalePreset", customToneIdx);
  }

  // ===== Typed OFX Param Accessors =====
  int getChoice(const char* name, double t, int def) const {
    if (auto* p = fetchChoiceParam(name)) {
      int v = def;
      p->getValueAtTime(t, v);
      return v;
    }
    return def;
  }
  int getInt(const char* name, double t, int def) const {
    if (auto* p = fetchIntParam(name)) return p->getValueAtTime(t);
    return def;
  }
  int getBool(const char* name, double t, int def) const {
    if (auto* p = fetchBooleanParam(name)) return p->getValueAtTime(t) ? 1 : 0;
    return def;
  }
  float getDouble(const char* name, double t, float def) const {
    if (auto* p = fetchDoubleParam(name)) return static_cast<float>(p->getValueAtTime(t));
    return def;
  }
  void setBool(const char* name, bool v) {
    if (auto* p = fetchBooleanParam(name)) p->setValue(v);
  }
  void setDouble(const char* name, double v) {
    if (auto* p = fetchDoubleParam(name)) p->setValue(v);
  }
  void setChoice(const char* name, int v) {
    if (auto* p = fetchChoiceParam(name)) p->setValue(v);
  }

  // ===== Look-Derived Defaults: base whitepoint and tonescale from selected look =====
  int selectedLookBaseCwp(double t) const {
    const int lookIdx = getChoice("lookPreset", t, 0);
    if (lookIdx < 0 || lookIdx >= static_cast<int>(kLookPresets.size())) return 2;
    return kLookPresets[static_cast<size_t>(lookIdx)].cwp;
  }

  float selectedLookBaseCwpLm(double t) const {
    const int lookIdx = getChoice("lookPreset", t, 0);
    if (lookIdx < 0 || lookIdx >= static_cast<int>(kLookPresets.size())) return 0.25f;
    return kLookPresets[static_cast<size_t>(lookIdx)].cwp_lm;
  }

  // ===== Menu Rebuild: reconstruct look/tonescale choice options =====
  void rebuildLookPresetMenuOptions(int preferredIndex) {
    auto* p = fetchChoiceParam("lookPreset");
    if (!p) return;
    p->resetOptions();
    for (const char* n : kLookPresetNames) p->appendOption(n);
    p->appendOption("(custom)");
    const int maxIndex = customLookPresetIndex();
    const int clamped = preferredIndex < 0 ? 0 : (preferredIndex > maxIndex ? maxIndex : preferredIndex);
    p->setValue(clamped);
  }

  void rebuildTonescalePresetMenuOptions(int preferredIndex) {
    auto* p = fetchChoiceParam("tonescalePreset");
    if (!p) return;
    p->resetOptions();
    for (const char* n : kTonescalePresetNames) p->appendOption(n);
    p->appendOption("(custom)");
    const int maxIndex = customTonescalePresetIndex();
    const int clamped = preferredIndex < 0 ? 0 : (preferredIndex > maxIndex ? maxIndex : preferredIndex);
    p->setValue(clamped);
  }

  void rebuildAllPresetMenus(int preferredLookIndex, int preferredToneIndex) {
    rebuildLookPresetMenuOptions(preferredLookIndex);
    rebuildTonescalePresetMenuOptions(preferredToneIndex);
  }

  // ===== Preset Menu Sync =====
  // Rebuild look/tonescale menus and refresh dependent preset state.
  void syncPresetMenus(double t, int preferredLookIndex, int preferredToneIndex) {
    int lookPreferred = preferredLookIndex;
    int tonePreferred = preferredToneIndex;
    if (lookPreferred < 0 || lookPreferred > customLookPresetIndex()) lookPreferred = 0;
    if (tonePreferred < 0 || tonePreferred > customTonescalePresetIndex()) tonePreferred = 0;
    rebuildAllPresetMenus(lookPreferred, tonePreferred);
    updatePresetStateFromCurrent(t);
    syncCombinedPresetMenuFromDisk(t, getChoice("userPresetCombined", t, 0));
  }

  void syncCombinedPresetMenuFromDisk(double t, int preferredIndex) {
    auto* p = fetchChoiceParam("userPresetCombined");
    if (!p) return;
    const std::vector<std::string> names = combinedPresetXmlNames();
    p->resetOptions();
    p->appendOption("None");
    for (const auto& n : names) p->appendOption(n);
    const int maxIndex = static_cast<int>(names.size());
    int clamped = preferredIndex;
    if (clamped < 0) clamped = 0;
    if (clamped > maxIndex) clamped = 0;
    p->setValue(clamped);
  }

  void setParamVisible(const char* name, bool visible) {
    if (!uiHostParamWritesSafeNow())
      return;
    try {
      if (auto* p = fetchGroupParam(name)) { p->setIsSecret(!visible); p->setEnabled(visible); return; }
      if (auto* p = fetchDoubleParam(name)) { p->setIsSecret(!visible); p->setEnabled(visible); return; }
      if (auto* p = fetchBooleanParam(name)) { p->setIsSecret(!visible); p->setEnabled(visible); return; }
      if (auto* p = fetchChoiceParam(name)) { p->setIsSecret(!visible); p->setEnabled(visible); return; }
      if (auto* p = fetchIntParam(name)) { p->setIsSecret(!visible); p->setEnabled(visible); return; }
      if (auto* p = fetchStringParam(name)) { p->setIsSecret(!visible); p->setEnabled(visible); return; }
    } catch (...) {
    }
  }

  void setParamEnabledOnly(const char* name, bool enabled) {
    if (!uiHostParamWritesSafeNow())
      return;
    try {
      if (auto* p = fetchDoubleParam(name)) { p->setEnabled(enabled); return; }
      if (auto* p = fetchBooleanParam(name)) { p->setEnabled(enabled); return; }
      if (auto* p = fetchChoiceParam(name)) { p->setEnabled(enabled); return; }
      if (auto* p = fetchIntParam(name)) { p->setEnabled(enabled); return; }
      if (auto* p = fetchStringParam(name)) { p->setEnabled(enabled); return; }
      if (auto* p = fetchPushButtonParam(name)) { p->setEnabled(enabled); return; }
      if (auto* p = fetchGroupParam(name)) { p->setEnabled(enabled); return; }
    } catch (...) {
    }
  }

  // Advanced toggle visibility updater.
  // Uses a small cache to avoid calling setIsSecret/setEnabled unless a driving toggle changed.
  void updateToggleVisibility(double t) {
    if (!uiHostParamWritesSafeNow()) {
      deferredHostUiFlush_.store(true, std::memory_order_release);
      return;
    }
    const bool whitePoint = getBool("wp_enable", t, 1) != 0;
    const bool toneGroup = getBool("tn_enable", t, 1) != 0;
    const bool renderGroup = getBool("rs_enable", t, 1) != 0;
    const bool hcon = getBool("tn_hcon_enable", t, 0) != 0;
    setParamVisible("grp_advanced_root", true);
    setParamVisible("reset_look_settings", true);
    setParamVisible("creativeWhitePreset", true);
    setParamVisible("cwp_lm", true);
    setParamVisible("reset_white_point", true);
    setParamEnabledOnly("creativeWhitePreset", whitePoint);
    setParamEnabledOnly("cwp_lm", whitePoint);
    setParamEnabledOnly("reset_white_point", whitePoint);
    setParamEnabledOnly("tn_con", toneGroup);
    setParamEnabledOnly("tn_sh", toneGroup);
    setParamEnabledOnly("tn_toe", toneGroup);
    setParamEnabledOnly("tn_off", toneGroup);
    setParamEnabledOnly("tn_hcon_enable", toneGroup);
    setParamEnabledOnly("tn_hcon", toneGroup && hcon);
    setParamEnabledOnly("tn_hcon_pv", toneGroup && hcon);
    setParamEnabledOnly("tn_hcon_st", toneGroup && hcon);
    setParamEnabledOnly("tn_lcon_enable", toneGroup);
    setParamEnabledOnly("tn_lcon", toneGroup && getBool("tn_lcon_enable", t, 0));
    setParamEnabledOnly("tn_lcon_w", toneGroup && getBool("tn_lcon_enable", t, 0));
    setParamEnabledOnly("reset_tonescale", toneGroup);
    setParamEnabledOnly("rs_sa", renderGroup);
    setParamEnabledOnly("rs_rw", renderGroup);
    setParamEnabledOnly("rs_bw", renderGroup);
    setParamEnabledOnly("reset_render_space", renderGroup);
    const bool lcon = getBool("tn_lcon_enable", t, 0) != 0;
    const bool pt = getBool("pt_enable", t, 1) != 0;
    const bool ptl = getBool("ptl_enable", t, 1) != 0;
    const bool ptm = getBool("ptm_enable", t, 1) != 0;
    const bool brl = getBool("brl_enable", t, 1) != 0;
    const bool brlp = getBool("brlp_enable", t, 1) != 0;
    const bool hc = getBool("hc_enable", t, 1) != 0;
    const bool hsRgb = getBool("hs_rgb_enable", t, 1) != 0;
    const bool hsCmy = getBool("hs_cmy_enable", t, 1) != 0;

    if (visibilityCacheInit_ &&
        hcon == vis_hcon_ &&
        lcon == vis_lcon_ &&
        pt == vis_pt_ &&
        ptl == vis_ptl_ &&
        ptm == vis_ptm_ &&
        brl == vis_brl_ &&
        brlp == vis_brlp_ &&
        hc == vis_hc_ &&
        hsRgb == vis_hsRgb_ &&
        hsCmy == vis_hsCmy_) {
      return;
    }

    const bool applyHcon = !visibilityCacheInit_ || hcon != vis_hcon_;
    const bool applyLcon = !visibilityCacheInit_ || lcon != vis_lcon_;
    const bool applyPt = !visibilityCacheInit_ || pt != vis_pt_;
    const bool applyPtl = !visibilityCacheInit_ || ptl != vis_ptl_;
    const bool applyPtm = !visibilityCacheInit_ || ptm != vis_ptm_;
    const bool applyBrl = !visibilityCacheInit_ || brl != vis_brl_;
    const bool applyBrlp = !visibilityCacheInit_ || brlp != vis_brlp_;
    const bool applyHc = !visibilityCacheInit_ || hc != vis_hc_;
    const bool applyHsRgb = !visibilityCacheInit_ || hsRgb != vis_hsRgb_;
    const bool applyHsCmy = !visibilityCacheInit_ || hsCmy != vis_hsCmy_;

    if (applyHcon) {
      const bool show = toneGroup && hcon;
      setParamVisible("tn_hcon", show);
      setParamVisible("tn_hcon_pv", show);
      setParamVisible("tn_hcon_st", show);
    }
    if (applyLcon) {
      const bool show = toneGroup && lcon;
      setParamVisible("tn_lcon", show);
      setParamVisible("tn_lcon_w", show);
    }
    if (applyPt) {
      setParamVisible("pt_lml", pt);
      setParamVisible("pt_lml_r", pt);
      setParamVisible("pt_lml_g", pt);
      setParamVisible("pt_lml_b", pt);
      setParamVisible("pt_lmh", pt);
      setParamVisible("pt_lmh_r", pt);
      setParamVisible("pt_lmh_b", pt);
    }
    if (applyPtl) {
      setParamVisible("ptl_c", ptl);
      setParamVisible("ptl_m", ptl);
      setParamVisible("ptl_y", ptl);
    }
    if (applyPtm) {
      setParamVisible("ptm_low", ptm);
      setParamVisible("ptm_low_rng", ptm);
      setParamVisible("ptm_low_st", ptm);
      setParamVisible("ptm_high", ptm);
      setParamVisible("ptm_high_rng", ptm);
      setParamVisible("ptm_high_st", ptm);
    }
    if (applyBrl) {
      setParamVisible("brl", brl);
      setParamVisible("brl_r", brl);
      setParamVisible("brl_g", brl);
      setParamVisible("brl_b", brl);
      setParamVisible("brl_rng", brl);
      setParamVisible("brl_st", brl);
    }
    if (applyBrlp) {
      setParamVisible("brlp", brlp);
      setParamVisible("brlp_r", brlp);
      setParamVisible("brlp_g", brlp);
      setParamVisible("brlp_b", brlp);
    }
    if (applyHc) {
      setParamVisible("hc_r", hc);
      setParamVisible("hc_r_rng", hc);
    }
    if (applyHsRgb) {
      setParamVisible("hs_r", hsRgb);
      setParamVisible("hs_r_rng", hsRgb);
      setParamVisible("hs_g", hsRgb);
      setParamVisible("hs_g_rng", hsRgb);
      setParamVisible("hs_b", hsRgb);
      setParamVisible("hs_b_rng", hsRgb);
    }
    if (applyHsCmy) {
      setParamVisible("hs_c", hsCmy);
      setParamVisible("hs_c_rng", hsCmy);
      setParamVisible("hs_m", hsCmy);
      setParamVisible("hs_m_rng", hsCmy);
      setParamVisible("hs_y", hsCmy);
      setParamVisible("hs_y_rng", hsCmy);
    }

    vis_hcon_ = hcon;
    vis_lcon_ = lcon;
    vis_pt_ = pt;
    vis_ptl_ = ptl;
    vis_ptm_ = ptm;
    vis_brl_ = brl;
    vis_brlp_ = brlp;
    vis_hc_ = hc;
    vis_hsRgb_ = hsRgb;
    vis_hsCmy_ = hsCmy;
    visibilityCacheInit_ = true;
  }
  void setInt(const char* name, int v) {
    if (auto* p = fetchIntParam(name)) p->setValue(v);
  }

  OpenDRTRawValues readRawValues(double time) const {
    OpenDRTRawValues r{};
    r.in_gamut = getChoice("in_gamut", time, 14);
    r.in_oetf = getChoice("in_oetf", time, 1);
    r.tn_Lp = getDouble("tn_Lp", time, 100.0f);
    r.tn_gb = getDouble("tn_gb", time, 0.13f);
    r.pt_hdr = getDouble("pt_hdr", time, 0.5f);
    r.tn_Lg = getDouble("tn_Lg", time, 10.0f);
    r.crv_enable = getBool("crv_enable", time, 0);
    r.lookPreset = getChoice("lookPreset", time, 0);
    r.tonescalePreset = getChoice("tonescalePreset", time, 0);
    r.creativeWhitePreset = getChoice("creativeWhitePreset", time, 2);
    r.cwp = getInt("cwp", time, 2);
    r.creativeWhiteLimit = getDouble("cwp_lm", time, 0.25f);
    if (!getBool("wp_enable", time, 1)) {
      r.cwp = selectedLookBaseCwp(time);
      r.creativeWhitePreset = r.cwp;
      r.creativeWhiteLimit = selectedLookBaseCwpLm(time);
    }
    r.displayEncodingPreset = getChoice("displayEncodingPreset", time, 0);

    r.tn_con = getDouble("tn_con", time, 1.66f);
    r.tn_sh = getDouble("tn_sh", time, 0.5f);
    r.tn_toe = getDouble("tn_toe", time, 0.003f);
    r.tn_off = getDouble("tn_off", time, 0.005f);
    r.tn_hcon_enable = getBool("tn_hcon_enable", time, 0);
    r.tn_hcon = getDouble("tn_hcon", time, 0.0f);
    r.tn_hcon_pv = getDouble("tn_hcon_pv", time, 1.0f);
    r.tn_hcon_st = getDouble("tn_hcon_st", time, 4.0f);
    r.tn_lcon_enable = getBool("tn_lcon_enable", time, 0);
    r.tn_lcon = getDouble("tn_lcon", time, 0.0f);
    r.tn_lcon_w = getDouble("tn_lcon_w", time, 0.5f);
    if (!getBool("tn_enable", time, 1)) {
      r.tn_hcon_enable = 0;
      r.tn_lcon_enable = 0;
    }

    r.rs_sa = getDouble("rs_sa", time, 0.35f);
    r.rs_rw = getDouble("rs_rw", time, 0.25f);
    r.rs_bw = getDouble("rs_bw", time, 0.55f);
    if (!getBool("rs_enable", time, 1)) {
      r.rs_sa = 0.0f;
    }

    r.pt_enable = getBool("pt_enable", time, 1);
    r.pt_lml = getDouble("pt_lml", time, 0.25f);
    r.pt_lml_r = getDouble("pt_lml_r", time, 0.5f);
    r.pt_lml_g = getDouble("pt_lml_g", time, 0.0f);
    r.pt_lml_b = getDouble("pt_lml_b", time, 0.1f);
    r.pt_lmh = getDouble("pt_lmh", time, 0.25f);
    r.pt_lmh_r = getDouble("pt_lmh_r", time, 0.5f);
    r.pt_lmh_b = getDouble("pt_lmh_b", time, 0.0f);
    r.ptl_enable = getBool("ptl_enable", time, 1);
    r.ptl_c = getDouble("ptl_c", time, 0.06f);
    r.ptl_m = getDouble("ptl_m", time, 0.08f);
    r.ptl_y = getDouble("ptl_y", time, 0.06f);
    r.ptm_enable = getBool("ptm_enable", time, 1);
    r.ptm_low = getDouble("ptm_low", time, 0.4f);
    r.ptm_low_rng = getDouble("ptm_low_rng", time, 0.25f);
    r.ptm_low_st = getDouble("ptm_low_st", time, 0.5f);
    r.ptm_high = getDouble("ptm_high", time, -0.8f);
    r.ptm_high_rng = getDouble("ptm_high_rng", time, 0.35f);
    r.ptm_high_st = getDouble("ptm_high_st", time, 0.4f);

    r.brl_enable = getBool("brl_enable", time, 1);
    r.brl = getDouble("brl", time, 0.0f);
    r.brl_r = getDouble("brl_r", time, -2.5f);
    r.brl_g = getDouble("brl_g", time, -1.5f);
    r.brl_b = getDouble("brl_b", time, -1.5f);
    r.brl_rng = getDouble("brl_rng", time, 0.5f);
    r.brl_st = getDouble("brl_st", time, 0.35f);
    r.brlp_enable = getBool("brlp_enable", time, 1);
    r.brlp = getDouble("brlp", time, -0.5f);
    r.brlp_r = getDouble("brlp_r", time, -1.25f);
    r.brlp_g = getDouble("brlp_g", time, -1.25f);
    r.brlp_b = getDouble("brlp_b", time, -0.25f);

    r.hc_enable = getBool("hc_enable", time, 1);
    r.hc_r = getDouble("hc_r", time, 1.0f);
    r.hc_r_rng = getDouble("hc_r_rng", time, 0.3f);
    r.hs_rgb_enable = getBool("hs_rgb_enable", time, 1);
    r.hs_r = getDouble("hs_r", time, 0.6f);
    r.hs_r_rng = getDouble("hs_r_rng", time, 0.6f);
    r.hs_g = getDouble("hs_g", time, 0.35f);
    r.hs_g_rng = getDouble("hs_g_rng", time, 1.0f);
    r.hs_b = getDouble("hs_b", time, 0.66f);
    r.hs_b_rng = getDouble("hs_b_rng", time, 1.0f);
    r.hs_cmy_enable = getBool("hs_cmy_enable", time, 1);
    r.hs_c = getDouble("hs_c", time, 0.25f);
    r.hs_c_rng = getDouble("hs_c_rng", time, 1.0f);
    r.hs_m = getDouble("hs_m", time, 0.0f);
    r.hs_m_rng = getDouble("hs_m_rng", time, 1.0f);
    r.hs_y = getDouble("hs_y", time, 0.0f);
    r.hs_y_rng = getDouble("hs_y_rng", time, 1.0f);

    r.clamp = getBool("clamp", time, 1);
    r.tn_su = getChoice("tn_su", time, 1);
    r.display_gamut = getChoice("display_gamut", time, 0);
    r.eotf = getChoice("eotf", time, 2);

    return r;
  }

  OFX::Clip* dstClip_ = nullptr;
  OFX::Clip* srcClip_ = nullptr;
  std::unique_ptr<OpenDRTProcessor> processor_;
  std::vector<float> srcPixels_;
  std::vector<float> dstPixels_;
#if defined(OFX_SUPPORTS_CUDARENDER)
  float* stageSrcPinned_ = nullptr;
  float* stageDstPinned_ = nullptr;
  size_t stagePinnedCapacityFloats_ = 0;
#endif
  bool suppressParamChanged_ = false;
  bool visibilityCacheInit_ = false;
  bool vis_hcon_ = false;
  bool vis_lcon_ = false;
  bool vis_pt_ = false;
  bool vis_ptl_ = false;
  bool vis_ptm_ = false;
  bool vis_brl_ = false;
  bool vis_brlp_ = false;
  bool vis_hc_ = false;
  bool vis_hsRgb_ = false;
  bool vis_hsCmy_ = false;
  bool allowUiParamWrites_ = true;
  std::atomic<int> hostRenderDepth_{0};
  std::atomic<bool> deferredHostUiFlush_{false};
};
