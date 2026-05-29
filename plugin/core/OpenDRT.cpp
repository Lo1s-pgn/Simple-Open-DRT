// OFX bootstrap: registers the plug-in factories for this bundle.
#include "ofxsImageEffect.h"

void openDRTRegisterFactories(OFX::PluginFactoryArray& ids);  // defined in OpenDRTEffect.cpp

void OFX::Plugin::getPluginIDs(OFX::PluginFactoryArray& ids) {
  openDRTRegisterFactories(ids);
}
