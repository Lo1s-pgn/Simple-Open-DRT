#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "OpenDRTPresets.h"

bool serializeLookValues(const LookPresetValues& v, std::string& out);
bool parseLookValues(const std::string& in, LookPresetValues* v);
bool serializeTonescaleValues(const TonescalePresetValues& v, std::string& out);
bool parseTonescaleValues(const std::string& in, TonescalePresetValues* v);

std::string xmlEscape(const std::string& in);
std::string xmlTagValue(const std::string& xml, const char* tag);
std::string combinedPresetFileStemFromPath(const std::string& path);
std::filesystem::path combinedUserPresetDirPath();
std::vector<std::string> combinedPresetXmlNames();
bool readCombinedPresetXmlFile(const std::filesystem::path& path, std::string* nameOut, LookPresetValues* lookOut, TonescalePresetValues* toneOut);
bool writeCombinedPresetXmlFile(const std::filesystem::path& path, const std::string& name, const LookPresetValues& look, const TonescalePresetValues& tone);
