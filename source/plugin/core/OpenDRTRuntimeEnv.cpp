#include "OpenDRTRuntimeEnv.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if !defined(__linux__)
#include <filesystem>
#endif
#include <fstream>
#include <mutex>

#if defined(_WIN32)
#include <windows.h>
#else
#if defined(__APPLE__) || defined(__linux__)
#include <cerrno>
#include <sys/stat.h>
#endif
#if defined(__APPLE__)
#endif
#endif

namespace OpenDRTRuntime {

namespace {

void appendMacDebugLogLine(const std::string& line) {
#if defined(__APPLE__)
  static std::mutex logMutex;
  static bool pathInit = false;
  static std::string logPath;
  if (!pathInit) {
    pathInit = true;
    const char* home = std::getenv("HOME");
    if (home && home[0] != '\0') {
      const std::string logsDir = std::string(home) + "/Library/Logs";
      (void)::mkdir(logsDir.c_str(), 0755);
      logPath = logsDir + "/Simple_Open_DRT.log";
    }
  }
  if (!logPath.empty()) {
    std::lock_guard<std::mutex> lock(logMutex);
    FILE* f = std::fopen(logPath.c_str(), "a");
    if (f != nullptr) {
      std::fprintf(f, "%s\n", line.c_str());
      std::fclose(f);
    }
  }
#else
  (void)line;
#endif
}

}  // namespace

bool perfLogEnabled() {
  static const bool enabled = []() {
    const char* v = std::getenv("SIMPLE_OPENDRT_PERF_LOG");
    if (v == nullptr || v[0] == '\0') return false;
    return !(v[0] == '0' && v[1] == '\0');
  }();
  return enabled;
}

bool forceStageCopyEnabled() {
  static const bool enabled = []() {
    const char* v = std::getenv("SIMPLE_OPENDRT_FORCE_STAGE_COPY");
    if (v == nullptr || v[0] == '\0') return false;
    return !(v[0] == '0' && v[1] == '\0');
  }();
  return enabled;
}

bool debugLogEnabled() {
  static const bool enabled = []() {
    const char* v = std::getenv("SIMPLE_OPENDRT_DEBUG_LOG");
    if (v == nullptr || v[0] == '\0') return false;
    return !(v[0] == '0' && v[1] == '\0');
  }();
  return enabled;
}

MetalRenderMode selectedMetalRenderMode() {
  static const MetalRenderMode mode = []() {
    const char* modeVar = std::getenv("SIMPLE_OPENDRT_METAL_RENDER_MODE");
    if (modeVar && modeVar[0] != '\0') {
      std::string m(modeVar);
      for (char& c : m) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      if (m == "INTERNAL") return MetalRenderMode::InternalOnly;
      if (m == "HOST" || m == "AUTO") return MetalRenderMode::HostPreferred;
    }
    return MetalRenderMode::HostPreferred;
  }();
  return mode;
}

#if defined(OFX_SUPPORTS_CUDARENDER)
CudaRenderMode selectedCudaRenderMode() {
  static const CudaRenderMode mode = []() {
    const char* modeVar = std::getenv("SIMPLE_OPENDRT_RENDER_MODE");
    if (modeVar && modeVar[0] != '\0') {
      std::string m(modeVar);
      for (char& c : m) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      if (m == "INTERNAL") return CudaRenderMode::InternalOnly;
      if (m == "HOST" || m == "AUTO") return CudaRenderMode::HostPreferred;
    }

    const char* forceInternal = std::getenv("SIMPLE_OPENDRT_FORCE_INTERNAL_PATH");
    if (forceInternal && forceInternal[0] != '\0' && !(forceInternal[0] == '0' && forceInternal[1] == '\0')) {
      return CudaRenderMode::InternalOnly;
    }
    const char* hostEnable = std::getenv("SIMPLE_OPENDRT_ENABLE_OFX_HOST_CUDA");
    if (hostEnable && hostEnable[0] != '\0' && !(hostEnable[0] == '0' && hostEnable[1] == '\0')) {
      return CudaRenderMode::HostPreferred;
    }

    return CudaRenderMode::HostPreferred;
  }();
  return mode;
}
#endif

void perfLog(const char* stage, const std::chrono::steady_clock::time_point& start) {
  if (!perfLogEnabled()) return;
  const auto now = std::chrono::steady_clock::now();
  const double ms = std::chrono::duration<double, std::milli>(now - start).count();
  std::fprintf(stderr, "[Simple_Open_DRT][PERF] %s: %.3f ms\n", stage, ms);
  {
    char buf[256];
    std::snprintf(buf, sizeof(buf), "[Simple_Open_DRT][PERF] %s: %.3f ms", stage, ms);
    appendMacDebugLogLine(buf);
  }
#if defined(_WIN32)
  static bool pathInit = false;
  static std::filesystem::path logPath;
  if (!pathInit) {
    pathInit = true;
    const char* base = std::getenv("LOCALAPPDATA");
    if (base && *base) {
      logPath = std::filesystem::path(base) / "Simple_Open_DRT" / "perf.log";
      std::error_code ec;
      std::filesystem::create_directories(logPath.parent_path(), ec);
    }
  }
  if (!logPath.empty()) {
    std::ofstream ofs(logPath, std::ios::app);
    if (ofs.is_open()) {
      ofs << "[Simple_Open_DRT][PERF] " << stage << ": " << ms << " ms\n";
    }
  }
#elif defined(__linux__)
  static bool pathInitLinux = false;
  static std::string logPathLinux;
  if (!pathInitLinux) {
    pathInitLinux = true;
    const char* home = std::getenv("HOME");
    if (home && *home) {
      const std::string cacheDir = std::string(home) + "/.cache";
      const std::string pluginDir = cacheDir + "/Simple_Open_DRT";
      (void)::mkdir(cacheDir.c_str(), 0755);
      (void)::mkdir(pluginDir.c_str(), 0755);
      logPathLinux = pluginDir + "/perf.log";
    } else {
      logPathLinux = "/tmp/Simple_Open_DRT_perf.log";
    }
  }
  if (!logPathLinux.empty()) {
    FILE* f = std::fopen(logPathLinux.c_str(), "a");
    if (f != nullptr) {
      std::fprintf(f, "[Simple_Open_DRT][PERF] %s: %.3f ms\n", stage, ms);
      std::fclose(f);
    }
  }
#endif
}

}  // namespace OpenDRTRuntime
