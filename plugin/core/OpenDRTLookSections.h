#pragma once

#include "OpenDRTParams.h"
#include "OpenDRTPresets.h"

#include "ofxsImageEffect.h"

namespace OpenDRTLookSections {

TonescalePresetValues captureTonescale(const OFX::ImageEffect& fx, double time);
LookPresetValues captureLook(const OFX::ImageEffect& fx, double time);

void applyWhitePoint(OFX::ImageEffect& fx, const OpenDRTParams& p);
void applyTonescale(OFX::ImageEffect& fx, const OpenDRTParams& p);
void applyRenderSpace(OFX::ImageEffect& fx, const OpenDRTParams& p);
void applyMidPurity(OFX::ImageEffect& fx, const OpenDRTParams& p);
void applyPurityCompression(OFX::ImageEffect& fx, const OpenDRTParams& p);
void applyBrilliance(OFX::ImageEffect& fx, const OpenDRTParams& p);
void applyHue(OFX::ImageEffect& fx, const OpenDRTParams& p);

}  // namespace OpenDRTLookSections
