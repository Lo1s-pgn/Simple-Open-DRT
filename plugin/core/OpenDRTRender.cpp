#include "OpenDRTRender.h"

OpenDRTRowLayout detectOpenDRTRowLayout(OFX::Image* img, const OfxRectI& bounds, int height, size_t rowBytes) {
  OpenDRTRowLayout out{};
  if (img == nullptr) return out;
  out.base = static_cast<float*>(img->getPixelAddress(bounds.x1, bounds.y1));
  if (out.base == nullptr) return out;
  if (height <= 1) {
    out.valid = true;
    out.contiguous = true;
    out.pitchBytes = rowBytes;
    return out;
  }
  const char* prev = reinterpret_cast<const char*>(out.base);
  std::ptrdiff_t step = 0;
  for (int y = bounds.y1 + 1; y < bounds.y2; ++y) {
    float* row = static_cast<float*>(img->getPixelAddress(bounds.x1, y));
    if (row == nullptr) return OpenDRTRowLayout{};
    const char* cur = reinterpret_cast<const char*>(row);
    if (y == bounds.y1 + 1) {
      step = cur - prev;
    } else if (cur - prev != step) {
      return OpenDRTRowLayout{};
    }
    prev = cur;
  }
  if (step <= 0) return OpenDRTRowLayout{};
  out.valid = true;
  out.pitchBytes = static_cast<size_t>(step);
  out.contiguous = (out.pitchBytes == rowBytes);
  return out;
}
