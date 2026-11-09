#include "er/profile.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <vector>

#include "engine/ini.h"

namespace er {

namespace {

std::vector<int> Ints(const std::string& s) {
  std::vector<int> v;
  std::stringstream ss(s);
  std::string part;
  while (std::getline(ss, part, ',')) v.push_back(std::atoi(part.c_str()));
  return v;
}

std::string Join(std::initializer_list<int> v) {
  std::string s;
  for (int x : v) s += (s.empty() ? "" : ",") + std::to_string(x);
  return s;
}

void SaveLeague(davi::IniFile& ini, const std::string& sec, const League& l) {
  ini.Set(sec, "new", std::to_string(l.is_new ? 1 : 0));
  ini.Set(sec, "round", std::to_string(l.round));
  ini.Set(sec, "player_club", std::to_string(l.player_club));
  ini.Set(sec, "opponents", Join({l.opponents[0], l.opponents[1], l.opponents[2], l.opponents[3]}));
  // Per club: money, points, rank, time, turbo, then damage,upgrade per class.
  for (int c = 0; c < 8; c++) {
    const ClubStats& s = l.clubs[c];
    ini.Set(sec, "club" + std::to_string(c),
            Join({s.money, s.score, s.rank, s.time, s.turbo, s.damage[0], s.upgrade[0], s.damage[1], s.upgrade[1],
                  s.damage[2], s.upgrade[2], s.damage[3], s.upgrade[3]}));
  }
}

void LoadLeague(const davi::IniFile& ini, const std::string& sec, League& l) {
  if (!ini.HasSection(sec)) return;
  l.is_new = ini.GetInt(sec, "new", 1) != 0;
  l.round = ini.GetInt(sec, "round", 0);
  l.player_club = ini.GetInt(sec, "player_club", 0);
  std::vector<int> o = Ints(ini.Get(sec, "opponents"));
  for (size_t i = 0; i < 4 && i < o.size(); i++) l.opponents[i] = o[i];
  for (int c = 0; c < 8; c++) {
    std::vector<int> v = Ints(ini.Get(sec, "club" + std::to_string(c)));
    if (v.size() < 13) continue;
    ClubStats& s = l.clubs[c];
    s.money = v[0], s.score = v[1], s.rank = v[2], s.time = v[3], s.turbo = v[4];
    for (int k = 0; k < 4; k++) s.damage[k] = v[5 + k * 2], s.upgrade[k] = v[6 + k * 2];
  }
}

}  // namespace

bool PlayerDb::Empty() const {
  for (auto& p : players)
    if (p.used) return false;
  return true;
}

bool PlayerDb::Load(const std::string& path) {
  davi::IniFile ini;
  if (!ini.Load(path)) return false;
  current = ini.GetInt("General", "current", 0);
  GlobalOptions& o = options;
  o.dx_index = ini.GetInt("Options", "dx_index", o.dx_index);
  o.resolution = ini.GetInt("Options", "resolution", o.resolution);
  o.brightness = ini.GetInt("Options", "brightness", o.brightness);
  o.mipmapping = ini.GetInt("Options", "mipmapping", o.mipmapping);
  o.texture_detail = ini.GetInt("Options", "texture_detail", o.texture_detail);
  o.filter = ini.GetInt("Options", "filter", o.filter);
  o.transparency = ini.GetInt("Options", "transparency", o.transparency);
  o.wait_vbl = ini.GetInt("Options", "wait_vbl", o.wait_vbl);
  o.speakers = ini.GetInt("Options", "speakers", o.speakers);
  o.sound_system = ini.GetInt("Options", "sound_system", o.sound_system);
  o.sound_quality = ini.GetInt("Options", "sound_quality", o.sound_quality);
  o.sound_on = ini.GetInt("Options", "sound_on", o.sound_on);
  o.hide_particles = ini.GetInt("Options", "hide_particles", o.hide_particles);
  o.hide_pedestrians = ini.GetInt("Options", "hide_pedestrians", o.hide_pedestrians);
  for (int i = 0; i < 5; i++) {
    std::string sec = "Player" + std::to_string(i);
    Profile& p = players[i];
    p = Profile{};
    if (!ini.HasSection(sec)) continue;
    p.used = ini.GetInt(sec, "used", 0) != 0;
    p.name = ini.Get(sec, "name").substr(0, 8);
    p.date = ini.Get(sec, "date");
    p.game_type = ini.GetInt(sec, "game_type", 0);
    p.ch_class = ini.GetInt(sec, "ch_class", 0);
    p.qr_type = ini.GetInt(sec, "qr_type", 0);
    p.qr_difficulty = ini.GetInt(sec, "qr_difficulty", 1);
    p.qr_track = ini.GetInt(sec, "qr_track", -1);
    p.qr_car = ini.GetInt(sec, "qr_car", -1);
    p.qr_opponent = ini.GetInt(sec, "qr_opponent", 0);
    p.ch_difficulty = ini.GetInt(sec, "ch_difficulty", 1);
    p.club = ini.GetInt(sec, "club", 0);
    p.ad_car = ini.GetInt(sec, "ad_car", -1);
    p.input_device = ini.GetInt(sec, "input_device", 0);
    p.show_controls = ini.GetInt(sec, "show_controls", 1);
    p.music = ini.GetInt(sec, "music", 50);
    p.sfx = ini.GetInt(sec, "sfx", 50);
    p.voices = ini.GetInt(sec, "voices", 50);
    for (int c = 0; c < 3; c++) LoadLeague(ini, sec + ".CH" + std::to_string(c), p.ch[c]);
    LoadLeague(ini, sec + ".AD", p.ad);
  }
  return true;
}

bool PlayerDb::Save(const std::string& path) const {
  davi::IniFile ini;
  auto set = [&](const std::string& sec, const char* key, int v) { ini.Set(sec, key, std::to_string(v)); };
  set("General", "current", current);
  const GlobalOptions& o = options;
  set("Options", "dx_index", o.dx_index);
  set("Options", "resolution", o.resolution);
  set("Options", "brightness", o.brightness);
  set("Options", "mipmapping", o.mipmapping);
  set("Options", "texture_detail", o.texture_detail);
  set("Options", "filter", o.filter);
  set("Options", "transparency", o.transparency);
  set("Options", "wait_vbl", o.wait_vbl);
  set("Options", "speakers", o.speakers);
  set("Options", "sound_system", o.sound_system);
  set("Options", "sound_quality", o.sound_quality);
  set("Options", "sound_on", o.sound_on);
  set("Options", "hide_particles", o.hide_particles);
  set("Options", "hide_pedestrians", o.hide_pedestrians);
  for (int i = 0; i < 5; i++) {
    const Profile& p = players[i];
    std::string sec = "Player" + std::to_string(i);
    set(sec, "used", p.used ? 1 : 0);
    ini.Set(sec, "name", p.name);
    ini.Set(sec, "date", p.date);
    set(sec, "game_type", p.game_type);
    set(sec, "ch_class", p.ch_class);
    set(sec, "qr_type", p.qr_type);
    set(sec, "qr_difficulty", p.qr_difficulty);
    set(sec, "qr_track", p.qr_track);
    set(sec, "qr_car", p.qr_car);
    set(sec, "qr_opponent", p.qr_opponent);
    set(sec, "ch_difficulty", p.ch_difficulty);
    set(sec, "club", p.club);
    set(sec, "ad_car", p.ad_car);
    set(sec, "input_device", p.input_device);
    set(sec, "show_controls", p.show_controls);
    set(sec, "music", p.music);
    set(sec, "sfx", p.sfx);
    set(sec, "voices", p.voices);
    if (!p.used) continue;
    for (int c = 0; c < 3; c++) SaveLeague(ini, sec + ".CH" + std::to_string(c), p.ch[c]);
    SaveLeague(ini, sec + ".AD", p.ad);
  }
  return ini.Save(path);
}

}  // namespace er
