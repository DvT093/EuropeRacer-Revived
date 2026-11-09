#include "er/gameini.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "er/game.h"
#include "er/table.h"

namespace er {

namespace {

int ClassIndex(const std::string& car_type) {
  if (car_type == "CT_ATV") return 1;
  if (car_type == "CT_SPORT") return 2;
  if (car_type == "CT_DOWNLOADED") return 3;
  return 0;
}

// [CLUBS] car of a class (0x49ee64).
int ClubCar(const Game& g, int club, int cls) {
  if (club < 0 || size_t(club) >= g.data.clubs.size()) return 0;
  const Club& c = g.data.clubs[club];
  return cls == 1 ? c.car_atv : cls == 2 ? c.car_sport : c.car_standard;
}

// The club that drives `car` (0x49ee98); 0 if none.
int ClubOfCar(const Game& g, int car) {
  for (size_t i = 0; i < g.data.clubs.size(); i++) {
    const Club& c = g.data.clubs[i];
    if (c.car_standard == car || c.car_sport == car || c.car_atv == car) return int(i);
  }
  return 0;
}

int UpgradeIndex(const std::string& s) {
  return s == "US_STAGE1" ? 1 : s == "US_STAGE2" ? 2 : s == "US_STAGE3" ? 3 : 0;
}

const QuickRaceTrack* QrRow(const Game& g, int track) {
  for (auto& q : g.data.quickrace)
    if (q.track == track) return &q;
  return nullptr;
}

int TrackId(Game& g) {
  if (g.choices.game_type == "QR") return g.choices.track;
  const RaceEntry* e = g.CurrentRace();
  return e ? e->track : 0;
}

// The player's car class (0x4a3828): QR the chosen car's, CH the chosen
// class, AD the adventure car's.
int PlayerClass(Game& g) {
  if (g.choices.game_type == "CH") return std::clamp(g.choices.ch_type, 0, 2);
  const Car* c = g.data.CarById(g.choices.car);
  return c ? ClassIndex(c->car_type) : 0;
}

void SetInt(davi::IniFile& ini, const char* sec, const std::string& key, int v) { ini.Set(sec, key, std::to_string(v)); }

}  // namespace

int CurrentRaceType(Game& g) {
  if (g.choices.game_type == "QR") return std::clamp(g.choices.qr_type, 0, 3);
  const RaceEntry* e = g.CurrentRace();
  return e && e->race_type == "QR_CHASE" ? kQrChase : kQrNormal;
}

void WriteGameIni(Game& g, davi::IniFile& ini) {
  const std::string& gt = g.choices.game_type;
  const int type = CurrentRaceType(g);
  const int track = TrackId(g);
  const Track* tr = g.data.TrackById(track);
  const int track_cls = tr ? ClassIndex(tr->car_type) : 0;  // TRACKS PreferredCarType
  const int cls = PlayerClass(g);
  League* league = g.CurrentLeague();

  // 0x49759c: start slots 0..4 shuffled; slot[0] (the player's) is not
  // written, slot[i+1] goes to racer i.
  int slot[5];
  bool used[5] = {};
  for (int i = 0; i < 5; i++) {
    int r = std::rand() % 4;
    for (int k = 0; k < 5 && used[r]; k++) r = (r + 1) % 5;
    used[r] = true;
    slot[i] = r;
  }

  // [PLAYER] / [ETAPPE] / [UPGRADES] PLAYER (0x4987d4).
  ini.Set("PLAYER", "Name", g.choices.player);
  SetInt(ini, "PLAYER", "Finished", 0);
  SetInt(ini, "PLAYER", "Money", 0);
  SetInt(ini, "ETAPPE", "Etappe", track);
  ini.Set("ETAPPE", "Name", tr ? g.texts(tr->name) : "");
  if (gt == "QR" || !league) {
    SetInt(ini, "PLAYER", "car", g.choices.car);
    SetInt(ini, "PLAYER", "country", ClubOfCar(g, g.choices.car));
    SetInt(ini, "PLAYER", "damage", 0);
    SetInt(ini, "PLAYER", "TURBO_AMOUNT", 100);
    SetInt(ini, "UPGRADES", "PLAYER", UpgradeIndex(g.data.qr_upgrade));
  } else {
    const ClubStats& me = league->clubs[league->player_club];
    SetInt(ini, "PLAYER", "car", ClubCar(g, league->player_club, cls));
    SetInt(ini, "PLAYER", "country", league->player_club);
    SetInt(ini, "PLAYER", "damage", me.damage[cls]);
    SetInt(ini, "PLAYER", "TURBO_AMOUNT", me.turbo);
    SetInt(ini, "UPGRADES", "PLAYER", me.upgrade[cls]);
  }

  // [OPTIONS] NrOpponents + [RACERS] (0x49902c, 0x4a34ac, 0x4988ec).
  static const int kOpponents[6] = {4, 1, 0, 4, 4, 0};
  const int n = kOpponents[type];
  SetInt(ini, "OPTIONS", "NrOpponents", n);
  auto racer = [&](int i, int car, int club, int upgrade) {
    char k[32];
    std::snprintf(k, sizeof k, "%d,%d,%d", car, slot[i + 1], 0);
    ini.Set("RACERS", std::to_string(i), k);
    std::snprintf(k, sizeof k, "country%.2d", i);
    SetInt(ini, "RACERS", k, club);
    std::snprintf(k, sizeof k, "Racer%d", i);
    SetInt(ini, "UPGRADES", k, upgrade);
  };
  if (gt == "QR" && (type == kQrTrack || type == kQrSuddenDeath)) {
    // 0x498c80: four random clubs, never the player's.
    bool taken[8] = {};
    int mine = ClubOfCar(g, g.choices.car);
    taken[mine] = true;
    for (int i = 0; i < n; i++) {
      int c = std::rand() % 8;
      for (int k = 0; k < 8 && taken[c]; k++) c = (c + 1) % 8;
      taken[c] = true;
      racer(i, ClubCar(g, c, track_cls), c, UpgradeIndex(g.data.qr_upgrade));
    }
  } else if (gt == "QR" && type == kQrDuel) {
    if (g.choices.opponent == 8) {
      racer(0, 44, 2, 0);  // J.B. Black: car 44 (not in [CARS]), country 2
    } else {
      int opp = g.choices.opponent, t = track_cls;
      // The same club in the same car class: the opponent takes another class.
      if (opp == ClubOfCar(g, g.choices.car) && cls == t) {
        static const int kOther[3][2] = {{1, 2}, {0, 2}, {1, 0}};
        t = kOther[std::clamp(t, 0, 2)][std::rand() % 2];
      }
      racer(0, ClubCar(g, opp, t), opp, 0);
    }
  } else if (league) {
    for (int i = 0; i < n && i < 4; i++) {
      int c = league->opponents[i];
      racer(i, ClubCar(g, c, track_cls), c, league->clubs[c].upgrade[cls]);
    }
  }

  // Race options + [BONUS] (0x499388, 0x499230).
  const RaceEntry* e = g.CurrentRace();
  SetInt(ini, "OPTIONS", "Difficulty", std::clamp(g.choices.difficulty, 0, 2));
  SetInt(ini, "OPTIONS", "GameType", gt == "CH" ? 1 : gt == "AD" ? 2 : 0);
  SetInt(ini, "OPTIONS", "RaceMode", type);
  int laps = 0;
  if (gt == "QR") {
    const QuickRaceTrack* q = QrRow(g, track);
    if (type == kQrTimeTrial) laps = g.data.qr_timetrial_laps;
    else if (q) laps = type == kQrTrack ? q->laps_single : type == kQrDuel ? q->laps_duel : q->laps_suddendeath;
  } else if (e) {
    laps = e->laps;
  }
  SetInt(ini, "OPTIONS", "Nrlaps", laps);
  SetInt(ini, "PLAYER", "lapcount", laps);
  for (int i = 1; i <= laps; i++) {
    char k[32];
    std::snprintf(k, sizeof k, "lap_%d_time", i);
    SetInt(ini, "PLAYER", k, 0x7fffffff);
  }
  bool qr = gt == "QR";
  SetInt(ini, "OPTIONS", "LocalCops", qr ? type == kQrTrack : e && e->cops);
  SetInt(ini, "OPTIONS", "EuroCops", qr ? type == kQrTrack : e && e->euro_cops);
  SetInt(ini, "OPTIONS", "SuperCops", qr ? 0 : e && e->super_cop);
  //                        TRACK DUEL TIME SUDDEN NORMAL CHASE
  static const int kTraffic[6] = {1, 0, 0, 0, 1, 1};
  static const int kOpp[6] = {1, 1, 0, 1, 1, 0};
  static const int kGhost[6] = {0, 0, 1, 0, 0, 0};
  static const int kBonus[6] = {0x0d, 0x0d, 0x05, 0x0d, 0x0f, 0x0f};  // repair 1, money 2, turbo 4, invulnerable 8
  SetInt(ini, "OPTIONS", "Traffic", kTraffic[type]);
  SetInt(ini, "OPTIONS", "Opponents", kOpp[type]);
  SetInt(ini, "OPTIONS", "Pedestrians", 1);  // forced 0 by an option (+0x379c) not built yet
  SetInt(ini, "OPTIONS", "ghostcar", kGhost[type]);
  SetInt(ini, "BONUS", "RepairBonus", (kBonus[type] & 1) != 0);
  SetInt(ini, "BONUS", "MoneyBonus", (kBonus[type] & 2) != 0);
  SetInt(ini, "BONUS", "TurboBonus", (kBonus[type] & 4) != 0);
  SetInt(ini, "BONUS", "InvulnerableBonus", (kBonus[type] & 8) != 0);

  // [GameSim] Qualify (0x499df0): the position to beat, or for a chase the
  // time (Qualify seconds as mm:ss:hh); "-" for Quick Race.
  std::string q = "-";
  if (!qr && e) {
    int d = std::clamp(g.choices.difficulty, 0, 2);
    q = type == kQrChase ? Msh(e->qualify[d] * 1000) : std::to_string(e->qualify[d]);
  }
  ini.Set("GameSim", "Qualify", q);
}

}  // namespace er
