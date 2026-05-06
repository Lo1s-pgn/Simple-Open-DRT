#pragma once
/* Session header on plug-in construct; errors append with mutex. Primary path: LSP/OpenDRT.log (see getLogPath). */
#include <chrono>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <mutex>
#include <string>
#if defined(__linux__)
#include <sys/stat.h>
#endif
#if defined(__APPLE__) || defined(__unix__)
#include <limits.h>
#include <sys/utsname.h>
#endif
#if defined(__APPLE__)
#include <dlfcn.h>
#include <sys/sysctl.h>
#endif
#if !defined(__linux__)
#include <filesystem>
#endif

namespace LSPOpenDRTLog {

inline std::mutex& getLogMutex() {
  static std::mutex s_mutex;
  return s_mutex;
}

inline std::string getLogPath() {
#if defined(_WIN32)
  const char* base = std::getenv("LOCALAPPDATA");
  if (base && base[0] != '\0') {
    std::string p(base);
    return p + "\\LSP\\OpenDRT.log";
  }
  return std::string("C:\\Temp\\OpenDRT.log");
#elif defined(__APPLE__)
  const char* home = std::getenv("HOME");
  if (!home || home[0] == '\0')
    return std::string("/tmp/OpenDRT.log");
  return std::string(home) + "/Library/Application Support/LSP/OpenDRT.log";
#elif defined(__linux__)
  const char* home = std::getenv("HOME");
  if (!home || home[0] == '\0')
    return std::string("/tmp/OpenDRT.log");
  const char* xdg = std::getenv("XDG_DATA_HOME");
  if (xdg && xdg[0] != '\0')
    return std::string(xdg) + "/LSP/OpenDRT.log";
  return std::string(home) + "/.local/share/LSP/OpenDRT.log";
#else
  const char* home = std::getenv("HOME");
  if (!home || home[0] == '\0')
    return std::string("/tmp/OpenDRT.log");
  return std::string(home) + "/Library/Application Support/LSP/OpenDRT.log";
#endif
}

inline std::string sanitizePathForLog(const std::string& p) {
  const char* home = std::getenv("HOME");
  if (!home || home[0] == '\0' || p.empty())
    return p;
  std::string hp(home);
  if (p.size() >= hp.size() && p.compare(0, hp.size(), hp) == 0
      && (p.size() == hp.size() || p[hp.size()] == '/' || p[hp.size()] == '\\'))
    return std::string("~") + p.substr(hp.size());
  return p;
}

#if defined(__linux__)
inline bool ensureLogDirectoryExists(const std::string& logPath) {
  size_t last = logPath.find_last_of('/');
  if (last == std::string::npos)
    return true;
  std::string dir = logPath.substr(0, last);
  struct stat st {};
  if (stat(dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    return true;
  size_t pos = 0;
  for (;;) {
    pos = dir.find('/', pos + 1);
    if (pos == std::string::npos)
      break;
    std::string sub = dir.substr(0, pos);
    if (mkdir(sub.c_str(), 0755) != 0 && errno != EEXIST)
      return false;
  }
  return (mkdir(dir.c_str(), 0755) == 0 || errno == EEXIST);
}
#else
inline bool ensureLogDirectoryExists(const std::string& logPath) {
  try {
    std::filesystem::path p(logPath);
    if (p.has_parent_path())
      std::filesystem::create_directories(p.parent_path());
    return true;
  } catch (...) {
    return false;
  }
}
#endif

inline std::string getTimestamp(const char* fmt = "%Y-%m-%d %H:%M:%S") {
  auto now = std::chrono::system_clock::now();
  auto t = std::chrono::system_clock::to_time_t(now);
  char buf[64];
  struct tm tm_buf {};
#if defined(_WIN32)
  localtime_s(&tm_buf, &t);
#else
  localtime_r(&t, &tm_buf);
#endif
  std::strftime(buf, sizeof(buf), fmt, &tm_buf);
  return std::string(buf);
}

inline bool openLogFile(std::ofstream& f, std::ios_base::openmode mode = std::ios::app) {
  std::string path = getLogPath();
  if (!ensureLogDirectoryExists(path))
    return false;
  f.open(path, mode);
  return f.good();
}

inline void ensureLogFileExists() {
  std::lock_guard<std::mutex> lock(getLogMutex());
  std::ofstream f;
  if (!openLogFile(f, std::ios::app))
    return;
  f.flush();
}

inline int getCoreCount() {
#if defined(__APPLE__)
  int ncpu = 0;
  size_t len = sizeof(ncpu);
  if (sysctlbyname("hw.ncpu", &ncpu, &len, NULL, 0) == 0)
    return ncpu;
#endif
  return 0;
}

#if defined(__APPLE__) || defined(__unix__)
inline void getUnameFields(
    std::string& sysname, std::string& nodename, std::string& release, std::string& version, std::string& machine) {
  struct utsname u {};
  sysname = nodename = release = version = machine = "unknown";
  if (uname(&u) == 0) {
    sysname = u.sysname;
    nodename = u.nodename;
    release = u.release;
    version = u.version;
    machine = u.machine;
  }
}
#endif

inline std::string getCpuInfo() {
#if defined(__APPLE__)
  char buf[256];
  size_t len = sizeof(buf);
  if (sysctlbyname("machdep.cpu.brand_string", buf, &len, NULL, 0) == 0 && len > 0)
    return std::string(buf, len - 1);
  len = sizeof(buf);
  if (sysctlbyname("hw.model", buf, &len, NULL, 0) == 0 && len > 0)
    return std::string(buf, len - 1);
#endif
  return "unknown";
}

inline std::string getMemoryInfoMB() {
#if defined(__APPLE__)
  uint64_t memsize = 0;
  size_t len = sizeof(memsize);
  if (sysctlbyname("hw.memsize", &memsize, &len, NULL, 0) == 0)
    return std::to_string(memsize / (1024 * 1024)) + " MB";
#endif
  return "unknown";
}

#if defined(__APPLE__)
static void openDrtLogBundleAnchor() {}

inline std::string getPluginBundleRootPath() {
  Dl_info info{};
  if (dladdr(reinterpret_cast<void*>(&openDrtLogBundleAnchor), &info) == 0 || !info.dli_fname)
    return "";
  std::string p = info.dli_fname;
  char resolved[PATH_MAX];
  if (realpath(p.c_str(), resolved))
    p = resolved;
  const char* marker = ".ofx.bundle";
  size_t pos = p.find(marker);
  if (pos == std::string::npos)
    return "";
  return p.substr(0, pos + std::strlen(marker));
}
#else
inline std::string getPluginBundleRootPath() {
  return "";
}
#endif

inline void writeSessionStart(const std::string& pluginName,
    const std::string& versionStr,
    const std::string& hostName,
    const std::string& hostLabel,
    const std::string& hostVersion,
    const std::string& buildInfo,
    const std::string& bundlePath,
    const std::string& gpuName) {
  std::lock_guard<std::mutex> lock(getLogMutex());
  std::ofstream f;
  if (!openLogFile(f, std::ios::out))
    return;
  const std::string bufTime = getTimestamp("%a %b %d %H:%M:%S %Y");

  f << "------------------------------------------------------------\n";
  f << "\t\t" << pluginName << "\n";
  f << "------------------------------------------------------------\n";
  f << "> Plugin Version\t: " << versionStr << "\n";
  f << "> Timestamp\t\t: " << bufTime << "\n";
  f << "> Host System Info: \n";

#if defined(__APPLE__) || defined(__unix__)
  std::string sysname, nodename, release, unameVersion, machine;
  getUnameFields(sysname, nodename, release, unameVersion, machine);
  (void)nodename;
  f << "\t- OS: \n";
  f << "\t\t" << sysname << "\n";
  f << "\t\t" << release << "\n";
  f << "\t\t" << unameVersion << "\n";
  f << "\t\t" << machine << "\n";
#endif
  f << "\t- CPU: \n\t\t" << getCpuInfo() << "\n";
  int cores = getCoreCount();
  if (cores > 0)
    f << "\t- Cores: \n\t\t" << cores << "\n";
  f << "\t- Memory: \n\t\t" << getMemoryInfoMB() << "\n";
  if (!gpuName.empty())
    f << "\t- GPU: \n\t\t" << gpuName << "\n";
  std::string hostDisplay = hostLabel.empty() ? hostName : hostLabel;
  if (hostDisplay.empty())
    hostDisplay = hostName.empty() ? "unknown" : hostName;
  f << "\t- Host: \n\t\t" << hostDisplay << "\n";
  if (!hostVersion.empty())
    f << "\t- Host version: \n\t\t" << hostVersion << "\n";
  if (!buildInfo.empty())
    f << "\t- Build: \n\t\t" << buildInfo << "\n";
  f << "\t- Renderer: \n\t\tMetal / CUDA / OpenCL / CPU (host-selected)\n";

  f << "\n------------------------------------------------------------\n";
  f << "[info] Logging start\n";
  if (!bundlePath.empty())
    f << "[info] Bundle path: " << sanitizePathForLog(bundlePath) << "/\n";
  f.flush();
}

inline void writeErrorLine(const std::string& message) {
  std::lock_guard<std::mutex> lock(getLogMutex());
  std::ofstream f;
  if (!openLogFile(f))
    return;
  f << getTimestamp() << " [error] " << message << "\n";
  f.flush();
}

inline void writeInfoLine(const std::string& message) {
  std::lock_guard<std::mutex> lock(getLogMutex());
  std::ofstream f;
  if (!openLogFile(f))
    return;
  f << getTimestamp() << " [info] " << message << "\n";
  f.flush();
}

} // namespace LSPOpenDRTLog

#define LSP_OPENDRT_LOG_ERROR(msg) LSPOpenDRTLog::writeErrorLine(std::string(msg))
#define LSP_OPENDRT_LOG_INFO(msg) LSPOpenDRTLog::writeInfoLine(std::string(msg))
#define LSP_OPENDRT_LOG_SESSION_START(n, v, hn, hl, hver, build, bundle, gpu) \
  LSPOpenDRTLog::writeSessionStart(std::string(n), std::string(v), std::string(hn), std::string(hl), std::string(hver), std::string(build), std::string(bundle), std::string(gpu))
