#pragma once

#include <chrono>
#include <string>

namespace OpenDRTRuntime {

bool perfLogEnabled();
bool forceStageCopyEnabled();
bool debugLogEnabled();

void perfLog(const char* stage, const std::chrono::steady_clock::time_point& start);

enum class MetalRenderMode {
  HostPreferred,
  InternalOnly
};
MetalRenderMode selectedMetalRenderMode();

#if defined(OFX_SUPPORTS_CUDARENDER)
enum class CudaRenderMode {
  HostPreferred,
  InternalOnly
};
CudaRenderMode selectedCudaRenderMode();
#endif

}  // namespace OpenDRTRuntime
