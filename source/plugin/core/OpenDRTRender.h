#pragma once

#include <cstddef>

#include "ofxsImageEffect.h"

struct OpenDRTRowLayout {
  bool valid = false;
  bool contiguous = false;
  float* base = nullptr;
  size_t pitchBytes = 0;
};

OpenDRTRowLayout detectOpenDRTRowLayout(OFX::Image* img, const OfxRectI& bounds, int height, size_t rowBytes);
