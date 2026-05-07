#include "OpenDRTPresetMenus.h"

#include "OpenDRTPresets.h"

namespace {
constexpr int kBuiltInLookPresetCount = static_cast<int>(kLookPresetNames.size());
constexpr int kBuiltInTonescalePresetCount = static_cast<int>(kTonescalePresetNames.size());
}  // namespace

int customLookPresetIndex() {
  return kBuiltInLookPresetCount;
}

int customTonescalePresetIndex() {
  return kBuiltInTonescalePresetCount;
}

bool isCustomLookPresetIndex(int idx) {
  return idx == customLookPresetIndex();
}

bool isCustomTonescalePresetIndex(int idx) {
  return idx == customTonescalePresetIndex();
}
