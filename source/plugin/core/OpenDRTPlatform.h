#pragma once

#include <string>

#if defined(__linux__)
std::string userPresetDirPath();
#else
#include <filesystem>
std::filesystem::path userPresetDirPath();
#endif

std::string pickOpenXmlFilePath();
std::string pickSaveXmlFilePath(const std::string& defaultName);
bool confirmOverwriteDialog(const std::string& presetName);
void showInfoDialog(const std::string& text);
bool openExternalUrl(const std::string& url);
