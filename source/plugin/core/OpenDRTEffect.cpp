#include <cmath>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <algorithm>
#if !defined(__linux__)
#include <filesystem>
#endif
#include <fstream>
#include <cstdio>
#include <atomic>
#include <mutex>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#include <spawn.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
extern char** environ;
#endif
#if defined(__linux__) || defined(__APPLE__)
#include <cerrno>
#include <sys/stat.h>
#endif

#include "ofxsImageEffect.h"

#if defined(OFX_SUPPORTS_CUDARENDER)
#include <cuda_runtime.h>
#endif

#include "OpenDRTConstants.h"
#include "OpenDRTRuntimeEnv.h"
#include "OpenDRTParams.h"
#include "OpenDRTPresets.h"
#include "OpenDRTProcessor.h"
#include "OpenDRTLog.h"
#include "OpenDRTDescribe.h"
#include "OpenDRTInterop.h"
#include "OpenDRTLookSections.h"
#include "OpenDRTPresetMenus.h"
#include "OpenDRTPlatform.h"
#include "OpenDRTRender.h"

bool openDrtIsAdvancedParam(const std::string& name);
bool openDrtIsTonescaleParam(const std::string& name);
bool openDrtIsVisibilityToggleParam(const std::string& name);
const char* openDrtTooltipForParam(const std::string& name);

#include "OpenDRTEffectBody.inl"

class OpenDRTFactory : public OFX::PluginFactoryHelper<OpenDRTFactory> {
 public:
  OpenDRTFactory() : PluginFactoryHelper<OpenDRTFactory>(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor) {}

  void load() override {}
  void unload() override {}

  void describe(OFX::ImageEffectDescriptor& d) override {
    static const std::string nameWithVersion = "LSP - Simple Open DRT v1.1.7";
    applyOpenDRTDescribeBasics(
        d,
        nameWithVersion,
        kPluginGrouping,
        std::string(kPluginDescription) + " | " + buildLabelText());

    bool advertiseHostCuda = false;
    bool advertiseHostMetal = false;
#if defined(OFX_SUPPORTS_CUDARENDER)
    advertiseHostCuda =
        (OpenDRTRuntime::selectedCudaRenderMode() == OpenDRTRuntime::CudaRenderMode::HostPreferred);
    advertiseHostMetal = false;
#elif defined(__APPLE__)
    advertiseHostMetal =
        (OpenDRTRuntime::selectedMetalRenderMode() == OpenDRTRuntime::MetalRenderMode::HostPreferred);
    advertiseHostCuda = false;
#endif
    applyOpenDRTHostRenderSupport(d, advertiseHostCuda, advertiseHostMetal);
  }

  void describeInContext(OFX::ImageEffectDescriptor& d, OFX::ContextEnum) override {
    describeOpenDRTInContext(d, openDrtTooltipForParam);
  }

  OFX::ImageEffect* createInstance(OfxImageEffectHandle h, OFX::ContextEnum) override { return new OpenDRTEffect(h); }
};

void openDRTRegisterFactories(OFX::PluginFactoryArray& ids) {
  static OpenDRTFactory factory;
  ids.push_back(&factory);
}
