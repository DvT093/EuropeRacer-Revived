#include "er/gamedata.h"

#include <cstdlib>

namespace er {

std::vector<std::string> Fields(const std::string& s) {
  std::vector<std::string> out;
  for (auto& p : davi::Split(s, ',')) out.push_back(davi::Trim(p));
  return out;
}

namespace {

int Num(const std::vector<std::string>& f, size_t i, int def = 0) {
  return i < f.size() && !f[i].empty() ? std::atoi(f[i].c_str()) : def;
}
std::string Str(const std::vector<std::string>& f, size_t i) { return i < f.size() ? f[i] : ""; }

// Rows "0=...", "1=..." in index order; stops at the first missing index so a
// mod can append row n+1.
template <class Fn>
void Rows(const davi::LayeredIni& ini, const std::string& sec, Fn fn) {
  for (int i = 0; ini.Has(sec, std::to_string(i)); i++) fn(i, Fields(ini.Get(sec, std::to_string(i))));
}

std::vector<RaceEntry> Races(const davi::LayeredIni& ini, const std::string& sec) {
  std::vector<RaceEntry> v;
  Rows(ini, sec, [&](int, const std::vector<std::string>& f) {
    RaceEntry r;
    r.track = Num(f, 0);
    r.laps = Num(f, 1);
    r.opponents = Num(f, 2);
    r.race_type = Str(f, 3);
    for (int d = 0; d < 3; d++) r.qualify[d] = Num(f, 4 + d);
    r.cops = Num(f, 7) != 0;
    r.euro_cops = Num(f, 8) != 0;
    r.super_cop = Num(f, 9) != 0;
    v.push_back(r);
  });
  return v;
}

std::map<std::string, std::vector<int>> Upgrades(const davi::LayeredIni& ini, const std::string& sec) {
  std::map<std::string, std::vector<int>> m;
  for (auto& k : ini.Keys(sec)) {
    std::vector<int> rounds;
    for (auto& f : Fields(ini.Get(sec, k))) rounds.push_back(std::atoi(f.c_str()));
    m[k] = rounds;
  }
  return m;
}

std::map<std::string, std::string> Map(const davi::LayeredIni& ini, const std::string& sec) {
  std::map<std::string, std::string> m;
  for (auto& k : ini.Keys(sec)) m[k] = davi::Trim(ini.Get(sec, k));
  return m;
}

}  // namespace

bool GameData::Load(const davi::LayeredIni& ini) {
  tracks.clear();
  Rows(ini, "TRACKS", [&](int i, const std::vector<std::string>& f) {
    tracks.push_back({i, Str(f, 0), Str(f, 1), Str(f, 2), Str(f, 3), Str(f, 4)});
  });
  quickrace.clear();
  Rows(ini, "QUICKRACE", [&](int, const std::vector<std::string>& f) {
    quickrace.push_back({Num(f, 0), Num(f, 1), Num(f, 2), Num(f, 3), Num(f, 4), Num(f, 5), Num(f, 6)});
  });
  championship = Races(ini, "CHAMPIONSHIP");
  adventure = Races(ini, "ADVENTURE");
  cars.clear();
  Rows(ini, "CARS", [&](int, const std::vector<std::string>& f) {
    Car c;
    c.id = Num(f, 0);
    c.car_type = Str(f, 1);
    c.info = Str(f, 2);
    for (size_t i = 3; i < f.size(); i++) c.props.push_back(Num(f, i));
    cars.push_back(c);
  });
  clubs.clear();
  Rows(ini, "CLUBS", [&](int, const std::vector<std::string>& f) {
    clubs.push_back({Str(f, 0), Str(f, 1), Str(f, 2), Str(f, 3), Str(f, 4), Str(f, 5),
                     Num(f, 6), Num(f, 7), Num(f, 8)});
  });
  ch_upgrades = Upgrades(ini, "CHUPGRADEPATH");
  ad_upgrades = Upgrades(ini, "ADUPGRADEPATH");
  sfx = Map(ini, "SOUND_EFFECT");
  voices = Map(ini, "SOUND_VOICE");
  music = Map(ini, "SOUND_MUSIC");
  qr_upgrade = davi::Trim(ini.Get("TWEAK", "QR_UPGRADE", "US_STAGE3"));
  qr_timetrial_laps = ini.GetInt("TWEAK", "QR_TIMETRIAL_NR_LAPS", 3);
  return !tracks.empty();
}

const Track* GameData::TrackById(int id) const {
  for (auto& t : tracks)
    if (t.id == id) return &t;
  return nullptr;
}

const Car* GameData::CarById(int id) const {
  for (auto& c : cars)
    if (c.id == id) return &c;
  return nullptr;
}

std::string Texts::operator()(const std::string& key) const {
  if (key.empty()) return "";
  if (ini_.Has(section_, key)) {
    // Language.ini writes line breaks as "\n".
    std::string v = ini_.Get(section_, key), out;
    for (size_t i = 0; i < v.size(); i++) {
      if (v[i] == '\\' && i + 1 < v.size() && (v[i + 1] == 'n' || v[i + 1] == 'N')) {
        out += '\n';
        i++;
      } else {
        out += v[i];
      }
    }
    return out;
  }
  return key;
}

std::string Texts::Caption(const std::string& value) const {
  std::string v = davi::Trim(value);
  return v.rfind("LT_", 0) == 0 ? (*this)(v) : v;
}

}  // namespace er
