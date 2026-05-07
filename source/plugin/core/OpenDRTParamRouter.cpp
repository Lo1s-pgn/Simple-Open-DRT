#include <string>

#include <vector>

bool openDrtIsAdvancedParam(const std::string& name) {
  static const std::vector<std::string> names = {
      "wp_enable",
      "tn_enable","tn_con","tn_sh","tn_toe","tn_off","tn_hcon_enable","tn_hcon","tn_hcon_pv","tn_hcon_st","tn_lcon_enable","tn_lcon","tn_lcon_w",
      "rs_enable","rs_sa","rs_rw","rs_bw",
      "pt_enable","pt_lml","pt_lml_r","pt_lml_g","pt_lml_b","pt_lmh","pt_lmh_r","pt_lmh_b","ptl_enable","ptl_c","ptl_m","ptl_y",
      "ptm_enable","ptm_low","ptm_low_rng","ptm_low_st","ptm_high","ptm_high_rng","ptm_high_st",
      "brl_enable","brl","brl_r","brl_g","brl_b","brl_rng","brl_st","brlp_enable","brlp","brlp_r","brlp_g","brlp_b",
      "hc_enable","hc_r","hc_r_rng","hs_rgb_enable","hs_r","hs_r_rng","hs_g","hs_g_rng","hs_b","hs_b_rng","hs_cmy_enable","hs_c","hs_c_rng","hs_m","hs_m_rng","hs_y","hs_y_rng",
      "clamp","tn_su","display_gamut","eotf","cwp","cwp_lm"};
  for (const auto& n : names) if (n == name) return true;
  return false;
}

bool openDrtIsTonescaleParam(const std::string& name) {
  static const std::vector<std::string> names = {
      "tn_con","tn_sh","tn_toe","tn_off","tn_hcon_enable","tn_hcon","tn_hcon_pv","tn_hcon_st","tn_lcon_enable","tn_lcon","tn_lcon_w"};
  for (const auto& n : names) if (n == name) return true;
  return false;
}

bool openDrtIsVisibilityToggleParam(const std::string& name) {
  static const std::vector<std::string> names = {
      "tn_hcon_enable","tn_lcon_enable",
      "pt_enable","ptl_enable","ptm_enable",
      "brl_enable","brlp_enable",
      "hc_enable","hs_rgb_enable","hs_cmy_enable"};
  for (const auto& n : names) if (n == name) return true;
  return false;
}
