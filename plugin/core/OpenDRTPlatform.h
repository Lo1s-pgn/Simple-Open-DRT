#pragma once

#include <filesystem>
#include <string>

std::filesystem::path userPresetDirPath();

std::string pickOpenXmlFilePath();
std::string pickSaveXmlFilePath(const std::string& defaultName);
bool confirmOverwriteDialog(const std::string& presetName);
void showInfoDialog(const std::string& text);
bool openExternalUrl(const std::string& url);
