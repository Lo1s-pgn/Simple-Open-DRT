#include "OpenDRTLookSections.h"

namespace {

int getIntValue(const OFX::ImageEffect& fx, const char* name, double t, int def) {
  if (auto* p = fx.fetchIntParam(name)) return p->getValueAtTime(t);
  return def;
}

int getBoolValue(const OFX::ImageEffect& fx, const char* name, double t, int def) {
  if (auto* p = fx.fetchBooleanParam(name)) return p->getValueAtTime(t) ? 1 : 0;
  return def;
}

float getDoubleValue(const OFX::ImageEffect& fx, const char* name, double t, float def) {
  if (auto* p = fx.fetchDoubleParam(name)) return static_cast<float>(p->getValueAtTime(t));
  return def;
}

}  // namespace

namespace OpenDRTLookSections {

TonescalePresetValues captureTonescale(const OFX::ImageEffect& fx, double time) {
  TonescalePresetValues t{};
  t.tn_con = getDoubleValue(fx, "tn_con", time, 1.66f);
  t.tn_sh = getDoubleValue(fx, "tn_sh", time, 0.5f);
  t.tn_toe = getDoubleValue(fx, "tn_toe", time, 0.003f);
  t.tn_off = getDoubleValue(fx, "tn_off", time, 0.005f);
  t.tn_hcon_enable = getBoolValue(fx, "tn_hcon_enable", time, 0);
  t.tn_hcon = getDoubleValue(fx, "tn_hcon", time, 0.0f);
  t.tn_hcon_pv = getDoubleValue(fx, "tn_hcon_pv", time, 1.0f);
  t.tn_hcon_st = getDoubleValue(fx, "tn_hcon_st", time, 4.0f);
  t.tn_lcon_enable = getBoolValue(fx, "tn_lcon_enable", time, 0);
  t.tn_lcon = getDoubleValue(fx, "tn_lcon", time, 0.0f);
  t.tn_lcon_w = getDoubleValue(fx, "tn_lcon_w", time, 0.5f);
  return t;
}

LookPresetValues captureLook(const OFX::ImageEffect& fx, double time) {
  LookPresetValues v{};
  v.tn_con = getDoubleValue(fx, "tn_con", time, 1.66f);
  v.tn_sh = getDoubleValue(fx, "tn_sh", time, 0.5f);
  v.tn_toe = getDoubleValue(fx, "tn_toe", time, 0.003f);
  v.tn_off = getDoubleValue(fx, "tn_off", time, 0.005f);
  v.tn_hcon_enable = getBoolValue(fx, "tn_hcon_enable", time, 0);
  v.tn_hcon = getDoubleValue(fx, "tn_hcon", time, 0.0f);
  v.tn_hcon_pv = getDoubleValue(fx, "tn_hcon_pv", time, 1.0f);
  v.tn_hcon_st = getDoubleValue(fx, "tn_hcon_st", time, 4.0f);
  v.tn_lcon_enable = getBoolValue(fx, "tn_lcon_enable", time, 0);
  v.tn_lcon = getDoubleValue(fx, "tn_lcon", time, 0.0f);
  v.tn_lcon_w = getDoubleValue(fx, "tn_lcon_w", time, 0.5f);
  v.cwp = getIntValue(fx, "cwp", time, 2);
  v.cwp_lm = getDoubleValue(fx, "cwp_lm", time, 0.25f);
  v.rs_sa = getDoubleValue(fx, "rs_sa", time, 0.35f);
  v.rs_rw = getDoubleValue(fx, "rs_rw", time, 0.25f);
  v.rs_bw = getDoubleValue(fx, "rs_bw", time, 0.55f);
  v.pt_enable = getBoolValue(fx, "pt_enable", time, 1);
  v.pt_lml = getDoubleValue(fx, "pt_lml", time, 0.25f);
  v.pt_lml_r = getDoubleValue(fx, "pt_lml_r", time, 0.5f);
  v.pt_lml_g = getDoubleValue(fx, "pt_lml_g", time, 0.0f);
  v.pt_lml_b = getDoubleValue(fx, "pt_lml_b", time, 0.1f);
  v.pt_lmh = getDoubleValue(fx, "pt_lmh", time, 0.25f);
  v.pt_lmh_r = getDoubleValue(fx, "pt_lmh_r", time, 0.5f);
  v.pt_lmh_b = getDoubleValue(fx, "pt_lmh_b", time, 0.0f);
  v.ptl_enable = getBoolValue(fx, "ptl_enable", time, 1);
  v.ptl_c = getDoubleValue(fx, "ptl_c", time, 0.06f);
  v.ptl_m = getDoubleValue(fx, "ptl_m", time, 0.08f);
  v.ptl_y = getDoubleValue(fx, "ptl_y", time, 0.06f);
  v.ptm_enable = getBoolValue(fx, "ptm_enable", time, 1);
  v.ptm_low = getDoubleValue(fx, "ptm_low", time, 0.4f);
  v.ptm_low_rng = getDoubleValue(fx, "ptm_low_rng", time, 0.25f);
  v.ptm_low_st = getDoubleValue(fx, "ptm_low_st", time, 0.5f);
  v.ptm_high = getDoubleValue(fx, "ptm_high", time, -0.8f);
  v.ptm_high_rng = getDoubleValue(fx, "ptm_high_rng", time, 0.35f);
  v.ptm_high_st = getDoubleValue(fx, "ptm_high_st", time, 0.4f);
  v.brl_enable = getBoolValue(fx, "brl_enable", time, 1);
  v.brl = getDoubleValue(fx, "brl", time, 0.0f);
  v.brl_r = getDoubleValue(fx, "brl_r", time, -2.5f);
  v.brl_g = getDoubleValue(fx, "brl_g", time, -1.5f);
  v.brl_b = getDoubleValue(fx, "brl_b", time, -1.5f);
  v.brl_rng = getDoubleValue(fx, "brl_rng", time, 0.5f);
  v.brl_st = getDoubleValue(fx, "brl_st", time, 0.35f);
  v.brlp_enable = getBoolValue(fx, "brlp_enable", time, 1);
  v.brlp = getDoubleValue(fx, "brlp", time, -0.5f);
  v.brlp_r = getDoubleValue(fx, "brlp_r", time, -1.25f);
  v.brlp_g = getDoubleValue(fx, "brlp_g", time, -1.25f);
  v.brlp_b = getDoubleValue(fx, "brlp_b", time, -0.25f);
  v.hc_enable = getBoolValue(fx, "hc_enable", time, 1);
  v.hc_r = getDoubleValue(fx, "hc_r", time, 1.0f);
  v.hc_r_rng = getDoubleValue(fx, "hc_r_rng", time, 0.3f);
  v.hs_rgb_enable = getBoolValue(fx, "hs_rgb_enable", time, 1);
  v.hs_r = getDoubleValue(fx, "hs_r", time, 0.6f);
  v.hs_r_rng = getDoubleValue(fx, "hs_r_rng", time, 0.6f);
  v.hs_g = getDoubleValue(fx, "hs_g", time, 0.35f);
  v.hs_g_rng = getDoubleValue(fx, "hs_g_rng", time, 1.0f);
  v.hs_b = getDoubleValue(fx, "hs_b", time, 0.66f);
  v.hs_b_rng = getDoubleValue(fx, "hs_b_rng", time, 1.0f);
  v.hs_cmy_enable = getBoolValue(fx, "hs_cmy_enable", time, 1);
  v.hs_c = getDoubleValue(fx, "hs_c", time, 0.25f);
  v.hs_c_rng = getDoubleValue(fx, "hs_c_rng", time, 1.0f);
  v.hs_m = getDoubleValue(fx, "hs_m", time, 0.0f);
  v.hs_m_rng = getDoubleValue(fx, "hs_m_rng", time, 1.0f);
  v.hs_y = getDoubleValue(fx, "hs_y", time, 0.0f);
  v.hs_y_rng = getDoubleValue(fx, "hs_y_rng", time, 1.0f);
  return v;
}

void applyWhitePoint(OFX::ImageEffect& fx, const OpenDRTParams& p) {
  if (auto* q = fx.fetchChoiceParam("creativeWhitePreset")) q->setValue(p.cwp);
  setIntIfPresent(fx, "cwp", p.cwp);
  setDoubleIfPresent(fx, "cwp_lm", p.cwp_lm);
}

void applyTonescale(OFX::ImageEffect& fx, const OpenDRTParams& p) {
  setDoubleIfPresent(fx, "tn_con", p.tn_con);
  setDoubleIfPresent(fx, "tn_sh", p.tn_sh);
  setDoubleIfPresent(fx, "tn_toe", p.tn_toe);
  setDoubleIfPresent(fx, "tn_off", p.tn_off);
  setBoolIfPresent(fx, "tn_hcon_enable", p.tn_hcon_enable != 0);
  setDoubleIfPresent(fx, "tn_hcon", p.tn_hcon);
  setDoubleIfPresent(fx, "tn_hcon_pv", p.tn_hcon_pv);
  setDoubleIfPresent(fx, "tn_hcon_st", p.tn_hcon_st);
  setBoolIfPresent(fx, "tn_lcon_enable", p.tn_lcon_enable != 0);
  setDoubleIfPresent(fx, "tn_lcon", p.tn_lcon);
  setDoubleIfPresent(fx, "tn_lcon_w", p.tn_lcon_w);
}

void applyRenderSpace(OFX::ImageEffect& fx, const OpenDRTParams& p) {
  setDoubleIfPresent(fx, "rs_sa", p.rs_sa);
  setDoubleIfPresent(fx, "rs_rw", p.rs_rw);
  setDoubleIfPresent(fx, "rs_bw", p.rs_bw);
}

void applyMidPurity(OFX::ImageEffect& fx, const OpenDRTParams& p) {
  setBoolIfPresent(fx, "ptm_enable", p.ptm_enable != 0);
  setDoubleIfPresent(fx, "ptm_low", p.ptm_low);
  setDoubleIfPresent(fx, "ptm_low_rng", p.ptm_low_rng);
  setDoubleIfPresent(fx, "ptm_low_st", p.ptm_low_st);
  setDoubleIfPresent(fx, "ptm_high", p.ptm_high);
  setDoubleIfPresent(fx, "ptm_high_rng", p.ptm_high_rng);
  setDoubleIfPresent(fx, "ptm_high_st", p.ptm_high_st);
}

void applyPurityCompression(OFX::ImageEffect& fx, const OpenDRTParams& p) {
  setBoolIfPresent(fx, "pt_enable", p.pt_enable != 0);
  setDoubleIfPresent(fx, "pt_lml", p.pt_lml);
  setDoubleIfPresent(fx, "pt_lml_r", p.pt_lml_r);
  setDoubleIfPresent(fx, "pt_lml_g", p.pt_lml_g);
  setDoubleIfPresent(fx, "pt_lml_b", p.pt_lml_b);
  setDoubleIfPresent(fx, "pt_lmh", p.pt_lmh);
  setDoubleIfPresent(fx, "pt_lmh_r", p.pt_lmh_r);
  setDoubleIfPresent(fx, "pt_lmh_b", p.pt_lmh_b);
  setBoolIfPresent(fx, "ptl_enable", p.ptl_enable != 0);
  setDoubleIfPresent(fx, "ptl_c", p.ptl_c);
  setDoubleIfPresent(fx, "ptl_m", p.ptl_m);
  setDoubleIfPresent(fx, "ptl_y", p.ptl_y);
}

void applyBrilliance(OFX::ImageEffect& fx, const OpenDRTParams& p) {
  setBoolIfPresent(fx, "brl_enable", p.brl_enable != 0);
  setDoubleIfPresent(fx, "brl", p.brl);
  setDoubleIfPresent(fx, "brl_r", p.brl_r);
  setDoubleIfPresent(fx, "brl_g", p.brl_g);
  setDoubleIfPresent(fx, "brl_b", p.brl_b);
  setDoubleIfPresent(fx, "brl_rng", p.brl_rng);
  setDoubleIfPresent(fx, "brl_st", p.brl_st);
  setBoolIfPresent(fx, "brlp_enable", p.brlp_enable != 0);
  setDoubleIfPresent(fx, "brlp", p.brlp);
  setDoubleIfPresent(fx, "brlp_r", p.brlp_r);
  setDoubleIfPresent(fx, "brlp_g", p.brlp_g);
  setDoubleIfPresent(fx, "brlp_b", p.brlp_b);
}

void applyHue(OFX::ImageEffect& fx, const OpenDRTParams& p) {
  setBoolIfPresent(fx, "hc_enable", p.hc_enable != 0);
  setDoubleIfPresent(fx, "hc_r", p.hc_r);
  setDoubleIfPresent(fx, "hc_r_rng", p.hc_r_rng);
  setBoolIfPresent(fx, "hs_rgb_enable", p.hs_rgb_enable != 0);
  setDoubleIfPresent(fx, "hs_r", p.hs_r);
  setDoubleIfPresent(fx, "hs_r_rng", p.hs_r_rng);
  setDoubleIfPresent(fx, "hs_g", p.hs_g);
  setDoubleIfPresent(fx, "hs_g_rng", p.hs_g_rng);
  setDoubleIfPresent(fx, "hs_b", p.hs_b);
  setDoubleIfPresent(fx, "hs_b_rng", p.hs_b_rng);
  setBoolIfPresent(fx, "hs_cmy_enable", p.hs_cmy_enable != 0);
  setDoubleIfPresent(fx, "hs_c", p.hs_c);
  setDoubleIfPresent(fx, "hs_c_rng", p.hs_c_rng);
  setDoubleIfPresent(fx, "hs_m", p.hs_m);
  setDoubleIfPresent(fx, "hs_m_rng", p.hs_m_rng);
  setDoubleIfPresent(fx, "hs_y", p.hs_y);
  setDoubleIfPresent(fx, "hs_y_rng", p.hs_y_rng);
}

}  // namespace OpenDRTLookSections
