#pragma once

#include <string>
#include <vector>

#include "ofxsImageEffect.h"

using OpenDRTTooltipFn = const char* (*)(const std::string&);

void applyOpenDRTDescribeBasics(
    OFX::ImageEffectDescriptor& d,
    const std::string& nameWithVersion,
    const char* pluginGrouping,
    const std::string& pluginDescriptionWithLabel);

void applyOpenDRTHostRenderSupport(
    OFX::ImageEffectDescriptor& d,
    bool advertiseHostCuda,
    bool advertiseHostMetal);

void describeOpenDRTInContext(
    OFX::ImageEffectDescriptor& d,
    OpenDRTTooltipFn tooltipFn);
