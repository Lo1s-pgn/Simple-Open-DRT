#include "OpenDRTInterop.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <system_error>

#include "OpenDRTPlatform.h"

namespace {
std::string sanitizePresetName(const std::string& s, const char* fallback) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    if (c == '\n' || c == '\r' || c == '\t') continue;
    out.push_back(c);
  }
  while (!out.empty() && out.front() == ' ') out.erase(out.begin());
  while (!out.empty() && out.back() == ' ') out.pop_back();
  if (out.empty()) out = fallback;
  if (out.size() > 96) out.resize(96);
  return out;
}
}  // namespace

bool serializeLookValues(const LookPresetValues& v, std::string& out) {
  std::ostringstream os;
  os.setf(std::ios::fixed);
  os.precision(9);
  os << v.tn_con << ' ' << v.tn_sh << ' ' << v.tn_toe << ' ' << v.tn_off << ' '
     << v.tn_hcon_enable << ' ' << v.tn_hcon << ' ' << v.tn_hcon_pv << ' ' << v.tn_hcon_st << ' '
     << v.tn_lcon_enable << ' ' << v.tn_lcon << ' ' << v.tn_lcon_w << ' '
     << v.cwp << ' ' << v.cwp_lm << ' '
     << v.rs_sa << ' ' << v.rs_rw << ' ' << v.rs_bw << ' '
     << v.pt_enable << ' '
     << v.pt_lml << ' ' << v.pt_lml_r << ' ' << v.pt_lml_g << ' ' << v.pt_lml_b << ' '
     << v.pt_lmh << ' ' << v.pt_lmh_r << ' ' << v.pt_lmh_b << ' '
     << v.ptl_enable << ' ' << v.ptl_c << ' ' << v.ptl_m << ' ' << v.ptl_y << ' '
     << v.ptm_enable << ' ' << v.ptm_low << ' ' << v.ptm_low_rng << ' ' << v.ptm_low_st << ' '
     << v.ptm_high << ' ' << v.ptm_high_rng << ' ' << v.ptm_high_st << ' '
     << v.brl_enable << ' ' << v.brl << ' ' << v.brl_r << ' ' << v.brl_g << ' ' << v.brl_b << ' '
     << v.brl_rng << ' ' << v.brl_st << ' '
     << v.brlp_enable << ' ' << v.brlp << ' ' << v.brlp_r << ' ' << v.brlp_g << ' ' << v.brlp_b << ' '
     << v.hc_enable << ' ' << v.hc_r << ' ' << v.hc_r_rng << ' '
     << v.hs_rgb_enable << ' ' << v.hs_r << ' ' << v.hs_r_rng << ' '
     << v.hs_g << ' ' << v.hs_g_rng << ' ' << v.hs_b << ' ' << v.hs_b_rng << ' '
     << v.hs_cmy_enable << ' ' << v.hs_c << ' ' << v.hs_c_rng << ' ' << v.hs_m << ' ' << v.hs_m_rng << ' '
     << v.hs_y << ' ' << v.hs_y_rng;
  out = os.str();
  return true;
}

bool parseLookValues(const std::string& in, LookPresetValues* v) {
  if (!v) return false;
  std::istringstream is(in);
  return static_cast<bool>(
    is >> v->tn_con >> v->tn_sh >> v->tn_toe >> v->tn_off
       >> v->tn_hcon_enable >> v->tn_hcon >> v->tn_hcon_pv >> v->tn_hcon_st
       >> v->tn_lcon_enable >> v->tn_lcon >> v->tn_lcon_w
       >> v->cwp >> v->cwp_lm
       >> v->rs_sa >> v->rs_rw >> v->rs_bw
       >> v->pt_enable
       >> v->pt_lml >> v->pt_lml_r >> v->pt_lml_g >> v->pt_lml_b
       >> v->pt_lmh >> v->pt_lmh_r >> v->pt_lmh_b
       >> v->ptl_enable >> v->ptl_c >> v->ptl_m >> v->ptl_y
       >> v->ptm_enable >> v->ptm_low >> v->ptm_low_rng >> v->ptm_low_st
       >> v->ptm_high >> v->ptm_high_rng >> v->ptm_high_st
       >> v->brl_enable >> v->brl >> v->brl_r >> v->brl_g >> v->brl_b
       >> v->brl_rng >> v->brl_st
       >> v->brlp_enable >> v->brlp >> v->brlp_r >> v->brlp_g >> v->brlp_b
       >> v->hc_enable >> v->hc_r >> v->hc_r_rng
       >> v->hs_rgb_enable >> v->hs_r >> v->hs_r_rng
       >> v->hs_g >> v->hs_g_rng >> v->hs_b >> v->hs_b_rng
       >> v->hs_cmy_enable >> v->hs_c >> v->hs_c_rng >> v->hs_m >> v->hs_m_rng
       >> v->hs_y >> v->hs_y_rng
  );
}

bool serializeTonescaleValues(const TonescalePresetValues& v, std::string& out) {
  std::ostringstream os;
  os.setf(std::ios::fixed);
  os.precision(9);
  os << v.tn_con << ' ' << v.tn_sh << ' ' << v.tn_toe << ' ' << v.tn_off << ' '
     << v.tn_hcon_enable << ' ' << v.tn_hcon << ' ' << v.tn_hcon_pv << ' ' << v.tn_hcon_st << ' '
     << v.tn_lcon_enable << ' ' << v.tn_lcon << ' ' << v.tn_lcon_w;
  out = os.str();
  return true;
}

bool parseTonescaleValues(const std::string& in, TonescalePresetValues* v) {
  if (!v) return false;
  std::istringstream is(in);
  return static_cast<bool>(
    is >> v->tn_con >> v->tn_sh >> v->tn_toe >> v->tn_off
       >> v->tn_hcon_enable >> v->tn_hcon >> v->tn_hcon_pv >> v->tn_hcon_st
       >> v->tn_lcon_enable >> v->tn_lcon >> v->tn_lcon_w
  );
}

std::string xmlEscape(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  for (char c : in) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&apos;"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

std::string xmlTagValue(const std::string& xml, const char* tag) {
  if (tag == nullptr) return std::string();
  const std::string open = std::string("<") + tag + ">";
  const std::string close = std::string("</") + tag + ">";
  const size_t begin = xml.find(open);
  if (begin == std::string::npos) return std::string();
  const size_t valueStart = begin + open.size();
  const size_t end = xml.find(close, valueStart);
  if (end == std::string::npos || end < valueStart) return std::string();
  return xml.substr(valueStart, end - valueStart);
}

std::string combinedPresetFileStemFromPath(const std::string& path) {
  const size_t slash = path.find_last_of("/\\");
  std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
  const size_t dot = name.find_last_of('.');
  if (dot != std::string::npos) name = name.substr(0, dot);
  return sanitizePresetName(name, "Preset");
}

std::filesystem::path combinedUserPresetDirPath() {
  return userPresetDirPath() / "combined_presets";
}

std::vector<std::string> combinedPresetXmlNames() {
  std::vector<std::string> out;
  std::error_code ec;
  const std::filesystem::path dir = combinedUserPresetDirPath();
  std::filesystem::create_directories(dir, ec);
  if (ec) return out;
  for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
    if (ec) break;
    if (!entry.is_regular_file()) continue;
    const std::filesystem::path p = entry.path();
    if (p.extension() != ".xml") continue;
    out.push_back(sanitizePresetName(p.stem().string(), "Preset"));
  }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

bool readCombinedPresetXmlFile(const std::filesystem::path& path, std::string* nameOut, LookPresetValues* lookOut, TonescalePresetValues* toneOut) {
  if (nameOut == nullptr || lookOut == nullptr || toneOut == nullptr) return false;
  std::ifstream is(path, std::ios::binary);
  if (!is.is_open()) return false;
  const std::string xml((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
  const std::string version = xmlTagValue(xml, "version");
  if (version.empty()) return false;
  const std::string name = sanitizePresetName(xmlTagValue(xml, "name"), path.stem().string().c_str());
  const std::string lookPayload = xmlTagValue(xml, "lookPayload");
  const std::string tonePayload = xmlTagValue(xml, "tonescalePayload");
  LookPresetValues look{};
  TonescalePresetValues tone{};
  if (!parseLookValues(lookPayload, &look)) return false;
  if (!parseTonescaleValues(tonePayload, &tone)) return false;
  *nameOut = name;
  *lookOut = look;
  *toneOut = tone;
  return true;
}

bool writeCombinedPresetXmlFile(const std::filesystem::path& path, const std::string& name, const LookPresetValues& look, const TonescalePresetValues& tone) {
  std::string lookPayload;
  std::string tonePayload;
  if (!serializeLookValues(look, lookPayload)) return false;
  if (!serializeTonescaleValues(tone, tonePayload)) return false;
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  std::ofstream os(path, std::ios::binary | std::ios::trunc);
  if (!os.is_open()) return false;
  os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  os << "<simpleOpenDrtPreset>\n";
  os << "  <version>1</version>\n";
  os << "  <name>" << xmlEscape(name) << "</name>\n";
  os << "  <lookPayload>" << xmlEscape(lookPayload) << "</lookPayload>\n";
  os << "  <tonescalePayload>" << xmlEscape(tonePayload) << "</tonescalePayload>\n";
  os << "</simpleOpenDrtPreset>\n";
  return os.good();
}
