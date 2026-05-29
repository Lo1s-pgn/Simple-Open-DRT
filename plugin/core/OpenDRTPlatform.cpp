#include "OpenDRTPlatform.h"

#include <cstdio>
#include <cstdlib>
#include <string>

std::filesystem::path userPresetDirPath() {
#ifdef _WIN32
  const char* base = std::getenv("APPDATA");
  if (!base || !*base) base = std::getenv("LOCALAPPDATA");
  if (base && *base) return std::filesystem::path(base) / "Simple_Open_DRT";
#elif defined(__APPLE__)
  const char* home = std::getenv("HOME");
  if (home && *home) return std::filesystem::path(home) / "Library" / "Application Support" / "Simple_Open_DRT";
#else
  const char* home = std::getenv("HOME");
  if (home && *home) return std::filesystem::path(home) / ".config" / "Simple_Open_DRT";
#endif
  return std::filesystem::path(".");
}

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

std::string pickOpenXmlFilePath() {
  char filePath[MAX_PATH] = {0};
  OPENFILENAMEA ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFilter = "XML Files (*.xml)\0*.xml\0All Files (*.*)\0*.*\0";
  ofn.lpstrFile = filePath;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
  ofn.lpstrDefExt = "xml";
  if (GetOpenFileNameA(&ofn) == TRUE) return std::string(filePath);
  return std::string();
}

std::string pickSaveXmlFilePath(const std::string& defaultName) {
  char filePath[MAX_PATH] = {0};
  std::snprintf(filePath, MAX_PATH, "%s", defaultName.c_str());
  OPENFILENAMEA ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFilter = "XML Files (*.xml)\0*.xml\0All Files (*.*)\0*.*\0";
  ofn.lpstrFile = filePath;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
  ofn.lpstrDefExt = "xml";
  if (GetSaveFileNameA(&ofn) == TRUE) return std::string(filePath);
  return std::string();
}

bool confirmOverwriteDialog(const std::string& presetName) {
  std::string msg = "Preset '" + presetName + "' already exists. Overwrite?";
  return MessageBoxA(nullptr, msg.c_str(), "Simple_Open_DRT", MB_ICONQUESTION | MB_YESNO) == IDYES;
}

void showInfoDialog(const std::string& text) {
  MessageBoxA(nullptr, text.c_str(), "Simple_Open_DRT", MB_ICONINFORMATION | MB_OK);
}

bool openExternalUrl(const std::string& url) {
  const HINSTANCE rc = ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  return reinterpret_cast<intptr_t>(rc) > 32;
}
#elif defined(__APPLE__)
std::string execAndRead(const std::string& cmd) {
  std::string out;
  FILE* f = popen(cmd.c_str(), "r");
  if (!f) return out;
  char buf[512];
  while (fgets(buf, sizeof(buf), f)) out += buf;
  pclose(f);
  while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
  return out;
}

std::string pickOpenXmlFilePath() {
  return execAndRead("osascript -e 'POSIX path of (choose file with prompt \"Import Simple_Open_DRT preset\" of type {\"public.xml\"})' 2>/dev/null");
}

std::string pickSaveXmlFilePath(const std::string& defaultName) {
  std::string cmd = "osascript -e 'POSIX path of (choose file name with prompt \"Export Simple_Open_DRT preset\" default name \"" + defaultName + "\")' 2>/dev/null";
  return execAndRead(cmd);
}

bool confirmOverwriteDialog(const std::string& presetName) {
  std::string cmd = "osascript -e 'button returned of (display dialog \"Preset \\\"" + presetName + "\\\" already exists. Overwrite?\" buttons {\"Cancel\",\"Overwrite\"} default button \"Overwrite\")' 2>/dev/null";
  return execAndRead(cmd) == "Overwrite";
}

void showInfoDialog(const std::string& text) {
  std::string esc = text;
  for (char& c : esc) if (c == '"') c = '\'';
  std::string cmd = "osascript -e 'display dialog \"" + esc + "\" buttons {\"OK\"} default button \"OK\"' 2>/dev/null";
  (void)execAndRead(cmd);
}

bool openExternalUrl(const std::string& url) {
  if (url.empty()) return false;
  std::string safe = url;
  for (char& c : safe) {
    if (c == '"') c = '\'';
  }
  std::string cmd = "open \"" + safe + "\" >/dev/null 2>&1";
  return std::system(cmd.c_str()) == 0;
}
#else
std::string execAndReadLinux(const std::string& cmd) {
  std::string out;
  FILE* f = popen(cmd.c_str(), "r");
  if (!f) return out;
  char buf[512];
  while (fgets(buf, sizeof(buf), f)) out += buf;
  pclose(f);
  while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
  return out;
}

bool linuxCommandExists(const char* cmd) {
  if (cmd == nullptr || cmd[0] == '\0') return false;
  std::string probe = "command -v ";
  probe += cmd;
  probe += " >/dev/null 2>&1";
  return std::system(probe.c_str()) == 0;
}

std::string pickOpenXmlFilePath() {
  if (linuxCommandExists("zenity")) {
    return execAndReadLinux("zenity --file-selection --title=\"Import Simple_Open_DRT preset\" --file-filter=\"*.xml\" 2>/dev/null");
  }
  if (linuxCommandExists("kdialog")) {
    return execAndReadLinux("kdialog --getopenfilename \"$HOME\" \"*.xml|XML Files\" 2>/dev/null");
  }
  return std::string();
}

std::string pickSaveXmlFilePath(const std::string& defaultName) {
  if (linuxCommandExists("zenity")) {
    std::string safe = defaultName;
    for (char& c : safe) if (c == '"') c = '\'';
    std::string cmd =
        "zenity --file-selection --save --confirm-overwrite --title=\"Export Simple_Open_DRT preset\" --filename=\"$HOME/" +
        safe + "\" 2>/dev/null";
    return execAndReadLinux(cmd);
  }
  if (linuxCommandExists("kdialog")) {
    std::string safe = defaultName;
    for (char& c : safe) if (c == '"') c = '\'';
    std::string cmd = "kdialog --getsavefilename \"$HOME/" + safe + "\" \"*.xml|XML Files\" 2>/dev/null";
    return execAndReadLinux(cmd);
  }
  return std::string();
}

bool confirmOverwriteDialog(const std::string& presetName) {
  if (linuxCommandExists("zenity")) {
    std::string safe = presetName;
    for (char& c : safe) if (c == '"') c = '\'';
    const std::string cmd = "zenity --question --title=\"Simple_Open_DRT\" --text=\"Preset '" + safe +
                            "' already exists. Overwrite?\" 2>/dev/null";
    return std::system(cmd.c_str()) == 0;
  }
  if (linuxCommandExists("kdialog")) {
    std::string safe = presetName;
    for (char& c : safe) if (c == '"') c = '\'';
    const std::string cmd = "kdialog --warningyesno \"Preset '" + safe + "' already exists. Overwrite?\" 2>/dev/null";
    return std::system(cmd.c_str()) == 0;
  }
  std::fprintf(stderr, "[Simple_Open_DRT] Linux fallback: overwrite confirmation unavailable for preset '%s'.\n", presetName.c_str());
  return false;
}

void showInfoDialog(const std::string& text) {
  if (linuxCommandExists("zenity")) {
    std::string safe = text;
    for (char& c : safe) if (c == '"') c = '\'';
    const std::string cmd = "zenity --info --title=\"Simple_Open_DRT\" --text=\"" + safe + "\" 2>/dev/null";
    (void)std::system(cmd.c_str());
    return;
  }
  if (linuxCommandExists("kdialog")) {
    std::string safe = text;
    for (char& c : safe) if (c == '"') c = '\'';
    const std::string cmd = "kdialog --msgbox \"" + safe + "\" 2>/dev/null";
    (void)std::system(cmd.c_str());
    return;
  }
  std::fprintf(stderr, "[Simple_Open_DRT] %s\n", text.c_str());
}

bool openExternalUrl(const std::string& url) {
  if (url.empty()) return false;
  if (!linuxCommandExists("xdg-open")) {
    std::fprintf(stderr, "[Simple_Open_DRT] Linux fallback: xdg-open not found.\n");
    return false;
  }
  std::string safe = url;
  for (char& c : safe) {
    if (c == '"') c = '\'';
  }
  const std::string cmd = "xdg-open \"" + safe + "\" >/dev/null 2>&1";
  return std::system(cmd.c_str()) == 0;
}
#endif
