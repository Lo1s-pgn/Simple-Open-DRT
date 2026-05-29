#include "OpenDRTDescribe.h"

#include <vector>

void applyOpenDRTDescribeBasics(
    OFX::ImageEffectDescriptor& d,
    const std::string& nameWithVersion,
    const char* pluginGrouping,
    const std::string& pluginDescriptionWithLabel) {
  d.setLabels(nameWithVersion.c_str(), nameWithVersion.c_str(), nameWithVersion.c_str());
  d.setPluginGrouping(pluginGrouping);
  d.setPluginDescription(pluginDescriptionWithLabel);
  d.addSupportedContext(OFX::eContextFilter);
  d.addSupportedBitDepth(OFX::eBitDepthFloat);
  d.setSingleInstance(false);
  d.setSupportsTiles(false);
  d.setSupportsMultiResolution(false);
  d.setTemporalClipAccess(false);
  d.setSupportsOpenCLBuffersRender(false);
}

void applyOpenDRTHostRenderSupport(
    OFX::ImageEffectDescriptor& d,
    bool advertiseHostCuda,
    bool advertiseHostMetal) {
#if defined(OFX_SUPPORTS_CUDARENDER)
  d.setSupportsCudaRender(advertiseHostCuda);
  d.setSupportsCudaStream(advertiseHostCuda);
#elif defined(__APPLE__)
  d.setSupportsMetalRender(advertiseHostMetal);
  d.setSupportsCudaRender(false);
  d.setSupportsCudaStream(false);
#else
  (void)advertiseHostMetal;
  d.setSupportsCudaRender(false);
  d.setSupportsCudaStream(false);
#endif
}

void describeOpenDRTInContext(
    OFX::ImageEffectDescriptor& d,
    OpenDRTTooltipFn tooltipFn) {
  auto hintFor = [&](const char* name) -> const char* {
    if (tooltipFn == nullptr) return nullptr;
    return tooltipFn(std::string(name));
  };

  OFX::ClipDescriptor* src = d.defineClip(kOfxImageEffectSimpleSourceClipName);
  src->addSupportedComponent(OFX::ePixelComponentRGBA);
  src->setTemporalClipAccess(false);
  src->setSupportsTiles(false);

  OFX::ClipDescriptor* dst = d.defineClip(kOfxImageEffectOutputClipName);
  dst->addSupportedComponent(OFX::ePixelComponentRGBA);
  dst->setSupportsTiles(false);

  auto* grpUserPresetsRoot = d.defineGroupParam("grp_user_presets_root");
  grpUserPresetsRoot->setLabel("USER PRESETS");
  grpUserPresetsRoot->setOpen(false);

  auto addChoice = [&](const char* name, const char* label, int def, const std::vector<const char*>& opts) {
    auto* p = d.defineChoiceParam(name);
    p->setLabel(label);
    for (const char* o : opts) p->appendOption(o);
    p->setDefault(def);
    if (const char* hint = hintFor(name)) p->setHint(hint);
    return p;
  };
  auto addDouble = [&](const char* name, const char* label, double def, double mn, double mx) {
    auto* p = d.defineDoubleParam(name);
    p->setLabel(label);
    p->setDefault(def);
    p->setRange(mn, mx);
    p->setDisplayRange(mn, mx);
    if (const char* hint = hintFor(name)) p->setHint(hint);
    return p;
  };

  auto* inGamut = addChoice("in_gamut", "Input Gamut", 14, {"XYZ","ACES 2065-1","ACEScg","P3D65","Rec.2020","Rec.709","Arri Wide Gamut 3","Arri Wide Gamut 4","Red Wide Gamut RGB","Sony SGamut3","Sony SGamut3Cine","Panasonic V-Gamut","Filmlight E-Gamut","Filmlight E-Gamut2","DaVinci Wide Gamut"});
  auto* inOetf = addChoice("in_oetf", "Input Transfer Function", 1, {"Linear","DaVinci Intermediate","Filmlight T-Log","ACEScct","Arri LogC3","Arri LogC4","RedLog3G10","Panasonic V-Log","Sony S-Log3","Fuji F-Log2"});

  auto* dep = addChoice("displayEncodingPreset", "Display Encoding Preset", 0, {"Rec.1886 - 2.4 Power / Rec.709","sRGB Display - 2.2 Power / Rec.709","Display P3 - 2.2 Power / P3-D65","DCI - 2.6 Power / P3-D60","DCI - 2.6 Power / P3-DCI","DCI - 2.6 Power / XYZ","Rec.2100 - PQ / Rec.2020","Rec.2100 - HLG / Rec.2020","Dolby - PQ / P3-D65"});
  auto* presetState = d.defineIntParam("presetState"); presetState->setIsSecret(true); presetState->setDefault(0);
  auto* cwpHidden = d.defineIntParam("cwp"); cwpHidden->setIsSecret(true); cwpHidden->setDefault(2);
  auto* grpBasicRoot = d.defineGroupParam("grp_basic_root");
  grpBasicRoot->setLabel("COLOR MANAGEMENT");
  grpBasicRoot->setOpen(true);
  inGamut->setParent(*grpBasicRoot);
  inOetf->setParent(*grpBasicRoot);
  dep->setParent(*grpBasicRoot);
  presetState->setParent(*grpBasicRoot);
  cwpHidden->setParent(*grpBasicRoot);
  auto* grpDisplay = d.defineGroupParam("grp_display"); grpDisplay->setLabel("Display Encoding"); grpDisplay->setOpen(false); grpDisplay->setParent(*grpBasicRoot);
  auto* grpPresetSelection = d.defineGroupParam("grp_preset_selection"); grpPresetSelection->setLabel("LOOK"); grpPresetSelection->setOpen(true);
  auto* resetLookSettings = d.definePushButtonParam("reset_look_settings");
  resetLookSettings->setLabel("Reset");
  resetLookSettings->setParent(*grpPresetSelection);
  auto* lookPreset = addChoice("lookPreset", "DRT Look Preset", 0, {"Standard","Arriba","Sylvan","Colorful","Aery","Dystopic","Umbra","Base"});
  lookPreset->appendOption("(custom)");
  auto* tonescalePreset = addChoice("tonescalePreset", "Tonescale Preset", 1, {"Low Contrast","Medium Contrast","High Contrast","Arriba Tonescale","Sylvan Tonescale","Colorful Tonescale","Aery Tonescale","Dystopic Tonescale","Umbra Tonescale","ACES-1.x","ACES-2.0","Marvelous Tonescape","DaGrinchi ToneGroan"});
  tonescalePreset->appendOption("(custom)");
  lookPreset->setParent(*grpPresetSelection);
  tonescalePreset->setParent(*grpPresetSelection);
  auto* grpAdvancedRoot = d.defineGroupParam("grp_advanced_root"); grpAdvancedRoot->setLabel("Custom Look"); grpAdvancedRoot->setOpen(false); grpAdvancedRoot->setParent(*grpPresetSelection);
  auto* grpWhitePoint = d.defineGroupParam("grp_white_point"); grpWhitePoint->setLabel("White Point"); grpWhitePoint->setOpen(false); grpWhitePoint->setParent(*grpAdvancedRoot);
  auto* wpEnable = d.defineBooleanParam("wp_enable");
  wpEnable->setLabel("Enable");
  wpEnable->setDefault(true);
  wpEnable->setIsSecret(true);
  auto* resetWhitePoint = d.definePushButtonParam("reset_white_point");
  resetWhitePoint->setLabel("Reset");
  resetWhitePoint->setParent(*grpWhitePoint);
  auto* cwpPreset = addChoice("creativeWhitePreset", "Creative White", 2, {"D93","D75","D65","D60","D55","D50"});
  auto* cwpLm = addDouble("cwp_lm", "Creative White Limit", 0.25, 0.0, 1.0);
  cwpPreset->setParent(*grpWhitePoint);
  cwpLm->setParent(*grpWhitePoint);
  auto* grpTone = d.defineGroupParam("grp_tonescale"); grpTone->setLabel("Tonescale"); grpTone->setOpen(false); grpTone->setParent(*grpAdvancedRoot);
  auto* grpRender = d.defineGroupParam("grp_render"); grpRender->setLabel("Render Space"); grpRender->setOpen(false); grpRender->setParent(*grpAdvancedRoot);
  auto* grpMidPurity = d.defineGroupParam("grp_mid_purity"); grpMidPurity->setLabel("Mid Purity"); grpMidPurity->setOpen(false); grpMidPurity->setParent(*grpAdvancedRoot);
  auto* grpPurityCompression = d.defineGroupParam("grp_purity_compression"); grpPurityCompression->setLabel("Purity Compression"); grpPurityCompression->setOpen(false); grpPurityCompression->setParent(*grpAdvancedRoot);
  auto* grpBrl = d.defineGroupParam("grp_brl"); grpBrl->setLabel("Brilliance"); grpBrl->setOpen(false); grpBrl->setParent(*grpAdvancedRoot);
  auto* grpHue = d.defineGroupParam("grp_hue"); grpHue->setLabel("Hue"); grpHue->setOpen(false); grpHue->setParent(*grpAdvancedRoot);

  auto addAdvBool = [&](const char* n, const char* l, bool def, OFX::GroupParamDescriptor* g){ auto* p=d.defineBooleanParam(n); p->setLabel(l); p->setDefault(def); p->setParent(*g); if (const char* hint = hintFor(n)) p->setHint(hint); return p; };
  auto addAdvD = [&](const char* n, const char* l, double df, double mn, double mx, OFX::GroupParamDescriptor* g){ auto* p=d.defineDoubleParam(n); p->setLabel(l); p->setDefault(df); p->setRange(mn,mx); p->setDisplayRange(mn,mx); p->setParent(*g); if (const char* hint = hintFor(n)) p->setHint(hint); return p; };
  auto addAdvC = [&](const char* n, const char* l, int df, const std::vector<const char*>& o, OFX::GroupParamDescriptor* g){ auto* p=d.defineChoiceParam(n); p->setLabel(l); for(auto* s:o)p->appendOption(s); p->setDefault(df); p->setParent(*g); if (const char* hint = hintFor(n)) p->setHint(hint); return p; };

  addAdvC("display_gamut","Display Gamut",0,{"Rec.709","P3-D65","Rec.2020","P3-D60","P3-DCI","XYZ"},grpDisplay);
  addAdvC("eotf","Display EOTF",2,{"Linear","2.2 Power sRGB","2.4 Power Rec.1886","2.6 Power DCI","ST 2084 PQ","HLG"},grpDisplay);
  addAdvD("tn_Lp", "Peak Luminance", 100.0, 100.0, 1000.0, grpDisplay);
  addAdvD("tn_Lg", "Grey Luminance", 10.0, 3.0, 25.0, grpDisplay);
  addAdvD("tn_gb", "HDR Grey Boost", 0.13, 0.0, 1.0, grpDisplay);
  addAdvD("pt_hdr", "HDR Purity", 0.5, 0.0, 1.0, grpDisplay);
  addAdvC("tn_su","Surround",1,{"Dark","Dim","Bright"},grpDisplay);
  addAdvBool("clamp","Clamp",true,grpDisplay);

  grpPresetSelection->setParent(*grpBasicRoot);

  auto* tnEnable = d.defineBooleanParam("tn_enable");
  tnEnable->setLabel("Enable");
  tnEnable->setDefault(true);
  tnEnable->setIsSecret(true);
  if (const char* hint = hintFor("tn_enable")) tnEnable->setHint(hint);
  auto* resetTonescale = d.definePushButtonParam("reset_tonescale");
  resetTonescale->setLabel("Reset");
  resetTonescale->setParent(*grpTone);
  auto* overlay = d.defineBooleanParam("crv_enable");
  overlay->setLabel("Tonescale Overlay");
  overlay->setDefault(false);
  if (const char* hint = hintFor("crv_enable")) overlay->setHint(hint);
  overlay->setParent(*grpTone);
  addAdvD("tn_con","Contrast",1.66,1.0,2.0,grpTone);
  addAdvD("tn_sh","Shoulder Clip",0.5,0.0,1.0,grpTone);
  addAdvD("tn_toe","Toe",0.003,0.0,0.1,grpTone);
  addAdvD("tn_off","Offset",0.005,0.0,0.02,grpTone);
  addAdvBool("tn_hcon_enable","Enable Contrast High",false,grpTone);
  addAdvD("tn_hcon","Contrast High",0.0,-1.0,1.0,grpTone);
  addAdvD("tn_hcon_pv","Contrast High Pivot",1.0,0.0,4.0,grpTone);
  addAdvD("tn_hcon_st","Contrast High Strength",4.0,0.0,4.0,grpTone);
  addAdvBool("tn_lcon_enable","Enable Contrast Low",false,grpTone);
  addAdvD("tn_lcon","Contrast Low",0.0,0.0,3.0,grpTone);
  addAdvD("tn_lcon_w","Contrast Low Width",0.5,0.0,2.0,grpTone);

  auto* rsEnable = d.defineBooleanParam("rs_enable");
  rsEnable->setLabel("Enable");
  rsEnable->setDefault(true);
  rsEnable->setIsSecret(true);
  if (const char* hint = hintFor("rs_enable")) rsEnable->setHint(hint);
  auto* resetRenderSpace = d.definePushButtonParam("reset_render_space");
  resetRenderSpace->setLabel("Reset");
  resetRenderSpace->setParent(*grpRender);
  addAdvD("rs_sa","Render Space Strength",0.35,0.0,0.6,grpRender);
  addAdvD("rs_rw","Render Space Weight R",0.25,0.0,0.8,grpRender);
  addAdvD("rs_bw","Render Space Weight B",0.55,0.0,0.8,grpRender);

  auto* ptmEnable = d.defineBooleanParam("ptm_enable");
  ptmEnable->setLabel("Enable");
  ptmEnable->setDefault(true);
  ptmEnable->setIsSecret(true);
  if (const char* hint = hintFor("ptm_enable")) ptmEnable->setHint(hint);
  auto* resetMidPurity = d.definePushButtonParam("reset_mid_purity");
  resetMidPurity->setLabel("Reset");
  resetMidPurity->setParent(*grpMidPurity);
  addAdvD("ptm_low","Mid Purity Low",0.4,0.0,2.0,grpMidPurity);
  addAdvD("ptm_low_rng","Mid Purity Low Range",0.25,0.0,1.0,grpMidPurity);
  addAdvD("ptm_low_st","Mid Purity Low Strength",0.5,0.1,1.0,grpMidPurity);
  addAdvD("ptm_high","Mid Purity High",-0.8,-0.9,0.0,grpMidPurity);
  addAdvD("ptm_high_rng","Mid Purity High Range",0.35,0.0,1.0,grpMidPurity);
  addAdvD("ptm_high_st","Mid Purity High Strength",0.4,0.1,1.0,grpMidPurity);

  auto* ptEnable = d.defineBooleanParam("pt_enable");
  ptEnable->setLabel("Enable");
  ptEnable->setDefault(true);
  ptEnable->setIsSecret(true);
  if (const char* hint = hintFor("pt_enable")) ptEnable->setHint(hint);
  auto* resetPurityCompression = d.definePushButtonParam("reset_purity_compression");
  resetPurityCompression->setLabel("Reset");
  resetPurityCompression->setParent(*grpPurityCompression);
  addAdvD("pt_lml","Purity Limit Low",0.25,0.0,1.0,grpPurityCompression);
  addAdvD("pt_lml_r","Purity Limit Low R",0.5,0.0,1.0,grpPurityCompression);
  addAdvD("pt_lml_g","Purity Limit Low G",0.0,0.0,1.0,grpPurityCompression);
  addAdvD("pt_lml_b","Purity Limit Low B",0.1,0.0,1.0,grpPurityCompression);
  addAdvD("pt_lmh","Purity Limit High",0.25,0.0,1.0,grpPurityCompression);
  addAdvD("pt_lmh_r","Purity Limit High R",0.5,0.0,1.0,grpPurityCompression);
  addAdvD("pt_lmh_b","Purity Limit High B",0.0,0.0,1.0,grpPurityCompression);
  addAdvBool("ptl_enable","Enable",true,grpPurityCompression);
  addAdvD("ptl_c","Purity Softclip C",0.06,0.0,0.25,grpPurityCompression);
  addAdvD("ptl_m","Purity Softclip M",0.08,0.0,0.25,grpPurityCompression);
  addAdvD("ptl_y","Purity Softclip Y",0.06,0.0,0.25,grpPurityCompression);

  auto* brlEnable = d.defineBooleanParam("brl_enable");
  brlEnable->setLabel("Enable");
  brlEnable->setDefault(true);
  brlEnable->setIsSecret(true);
  if (const char* hint = hintFor("brl_enable")) brlEnable->setHint(hint);
  auto* resetBrilliance = d.definePushButtonParam("reset_brilliance");
  resetBrilliance->setLabel("Reset");
  resetBrilliance->setParent(*grpBrl);
  addAdvD("brl","Brilliance",0.0,-6.0,2.0,grpBrl);
  addAdvD("brl_r","Brilliance R",-2.5,-6.0,2.0,grpBrl);
  addAdvD("brl_g","Brilliance G",-1.5,-6.0,2.0,grpBrl);
  addAdvD("brl_b","Brilliance B",-1.5,-6.0,2.0,grpBrl);
  addAdvD("brl_rng","Brilliance Range",0.5,0.0,1.0,grpBrl);
  addAdvD("brl_st","Brilliance Strength",0.35,0.0,1.0,grpBrl);
  addAdvBool("brlp_enable","Enable",true,grpBrl);
  addAdvD("brlp","Brilliance Post",-0.5,-1.0,0.0,grpBrl);
  addAdvD("brlp_r","Post Brilliance R",-1.25,-3.0,0.0,grpBrl);
  addAdvD("brlp_g","Post Brilliance G",-1.25,-3.0,0.0,grpBrl);
  addAdvD("brlp_b","Post Brilliance B",-0.25,-3.0,0.0,grpBrl);

  auto* hcEnable = d.defineBooleanParam("hc_enable");
  hcEnable->setLabel("Enable");
  hcEnable->setDefault(true);
  hcEnable->setIsSecret(true);
  if (const char* hint = hintFor("hc_enable")) hcEnable->setHint(hint);
  auto* resetHue = d.definePushButtonParam("reset_hue");
  resetHue->setLabel("Reset");
  resetHue->setParent(*grpHue);
  addAdvD("hc_r","Hue Contrast R",1.0,0.0,2.0,grpHue);
  addAdvD("hc_r_rng","Hue Contrast R Range",0.3,0.0,1.0,grpHue);
  addAdvBool("hs_rgb_enable","Enable",true,grpHue);
  addAdvD("hs_r","Hueshift R",0.6,0.0,1.0,grpHue);
  addAdvD("hs_g","Hueshift G",0.35,0.0,1.0,grpHue);
  addAdvD("hs_b","Hueshift B",0.66,0.0,1.0,grpHue);
  addAdvD("hs_r_rng","Hueshift R Range",0.6,0.0,2.0,grpHue);
  addAdvD("hs_g_rng","Hueshift G Range",1.0,0.0,2.0,grpHue);
  addAdvD("hs_b_rng","Hueshift B Range",1.0,0.0,4.0,grpHue);
  addAdvBool("hs_cmy_enable","Enable",true,grpHue);
  addAdvD("hs_c","Hueshift C",0.25,0.0,1.0,grpHue);
  addAdvD("hs_m","Hueshift M",0.0,0.0,1.0,grpHue);
  addAdvD("hs_y","Hueshift Y",0.0,0.0,1.0,grpHue);
  addAdvD("hs_c_rng","Hueshift C Range",1.0,0.0,1.0,grpHue);
  addAdvD("hs_m_rng","Hueshift M Range",1.0,0.0,1.0,grpHue);
  addAdvD("hs_y_rng","Hueshift Y Range",1.0,0.0,1.0,grpHue);

  auto* userPresetCombined = d.defineChoiceParam("userPresetCombined");
  userPresetCombined->setLabel("Preset");
  userPresetCombined->appendOption("None");
  userPresetCombined->setParent(*grpUserPresetsRoot);

  auto* userPresetExportXml = d.definePushButtonParam("userPresetExportXml");
  userPresetExportXml->setLabel("Export Preset");
  userPresetExportXml->setParent(*grpUserPresetsRoot);

  auto* userPresetImportXml = d.definePushButtonParam("userPresetImportXml");
  userPresetImportXml->setLabel("Import Preset");
  userPresetImportXml->setParent(*grpUserPresetsRoot);

  auto* userPresetRefreshXml = d.definePushButtonParam("userPresetRefreshXml");
  userPresetRefreshXml->setLabel("Refresh Presets");
  userPresetRefreshXml->setParent(*grpUserPresetsRoot);

  auto* grpSupportRoot = d.defineGroupParam("grp_support_root");
  grpSupportRoot->setLabel("SUPPORT");
  grpSupportRoot->setOpen(false);
  grpSupportRoot->setParent(*grpBasicRoot);

  auto* creditsLabel1 = d.defineStringParam("creditsLabel1");
  creditsLabel1->setLabel("Credits 1");
  creditsLabel1->setDefault("Based on Open DRT by Jed Smith");
  creditsLabel1->setStringType(OFX::eStringTypeLabel);
  creditsLabel1->setEnabled(false);
  creditsLabel1->setParent(*grpSupportRoot);

  auto* creditsLabel2 = d.defineStringParam("creditsLabel2");
  creditsLabel2->setLabel("Credits 2");
  creditsLabel2->setDefault("OFX by Loïs Plagnard");
  creditsLabel2->setStringType(OFX::eStringTypeLabel);
  creditsLabel2->setEnabled(false);
  creditsLabel2->setParent(*grpSupportRoot);

  auto* supportPortedVersion = d.defineStringParam("supportPortedVersion");
  supportPortedVersion->setLabel("OpenDRT Base Version");
  supportPortedVersion->setDefault("V1.1.0");
  supportPortedVersion->setEnabled(false);
  supportPortedVersion->setParent(*grpSupportRoot);

  auto* supportHelp = d.definePushButtonParam("supportHelp");
  supportHelp->setLabel("Help");
  supportHelp->setParent(*grpSupportRoot);

  auto* supportOpenLog = d.definePushButtonParam("supportOpenLog");
  supportOpenLog->setLabel("Open Log");
  supportOpenLog->setHint("Open OpenDRT.log in the LSP application support folder (see plug-in documentation for OS paths).");
  supportOpenLog->setParent(*grpSupportRoot);

  auto* supportReportIssue = d.definePushButtonParam("supportReportIssue");
  supportReportIssue->setLabel("Report Issue");
  supportReportIssue->setParent(*grpSupportRoot);

  auto* supportOpenDrtRepo = d.definePushButtonParam("supportOpenDrtRepo");
  supportOpenDrtRepo->setLabel("OpenDRT");
  supportOpenDrtRepo->setParent(*grpSupportRoot);
}
