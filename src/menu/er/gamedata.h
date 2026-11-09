// Europe Racer menu: game data from the game's own GameMenu.ini (tracks,
// race tables, cars, clubs, upgrade paths, sounds) and the texts from
// Langdata/<lang>/menu/Language.ini. A mod adds or changes an entry with an
// ini layer on top; nothing here is hard-coded per track or car.
//
// Source of the formats: the comments in GameMenu.ini itself (Davilex wrote
// the record layouts there) and GameMenu.dat (Delphi, GameMenuUtils.pas).
#pragma once

#include <map>
#include <string>
#include <vector>

#include "engine/ini.h"

namespace er {

// GameMenu.ini enums, kept as the strings the ini uses.
// Track region: TC_WESTEUROPE / TC_MIDDLEEUROPE / TC_SOUTHEUROPE.
// Car class:    CT_STANDARD / CT_SPORT / CT_ATV.
// Race type:    QR_NORMAL / QR_CHASE.

// [TRACKS] n=Region,CarType,InfoText,QuickRaceInfoText,NameText
struct Track {
  int id = 0;
  std::string region, car_type, info, qr_info, name;  // texts are LT_ keys
};

// [QUICKRACE] n=TrackID, laps single, laps duel, laps time trial,
//   laps sudden death, opponents single, opponents sudden death
struct QuickRaceTrack {
  int track = 0;
  int laps_single = 0, laps_duel = 0, laps_timetrial = 0, laps_suddendeath = 0;
  int opp_single = 0, opp_suddendeath = 0;
};

// [CHAMPIONSHIP] / [ADVENTURE] n=TrackID, laps, opponents, race type,
//   qualify easy/normal/hard, normal cops, euro cops, super cop
// Qualify is a position for QR_NORMAL and a time in seconds for QR_CHASE.
struct RaceEntry {
  int track = 0, laps = 0, opponents = 0;
  std::string race_type;
  int qualify[3] = {0, 0, 0};
  bool cops = false, euro_cops = false, super_cop = false;
};

// [CARS] n=CarID, CarType, InfoText, 16 property values
// (TCarInfo.CarProperty: array [TUpgradeType][TCarProperty], 4 x 4).
struct Car {
  int id = 0;  // game car id (game.ini [PLAYER] car)
  std::string car_type, info;
  std::vector<int> props;
};

// [CLUBS] n=ClubType, Name, Info, OpponentName, OpponentInfo, ClubCar(LT_CD_),
//   Standard car, Sport car, ATV car (row numbers in [CARS] order as ids)
struct Club {
  std::string type, name, info, opponent, opponent_info, car_text;
  int car_standard = 0, car_sport = 0, car_atv = 0;
};

struct GameData {
  std::vector<Track> tracks;
  std::vector<QuickRaceTrack> quickrace;
  std::vector<RaceEntry> championship, adventure;
  std::vector<Car> cars;
  std::vector<Club> clubs;
  // [CHUPGRADEPATH] / [ADUPGRADEPATH]: club type -> 9 rounds
  // (standard 1st..3rd, ATV 1st..3rd, sport 1st..3rd).
  std::map<std::string, std::vector<int>> ch_upgrades, ad_upgrades;
  // [SOUND_EFFECT] / [SOUND_VOICE] / [SOUND_MUSIC]: id -> file stem.
  std::map<std::string, std::string> sfx, voices, music;
  // [TWEAK]
  std::string qr_upgrade = "US_STAGE3";
  int qr_timetrial_laps = 3;

  bool Load(const davi::LayeredIni& ini);
  const Track* TrackById(int id) const;
  const Car* CarById(int id) const;
};

// Language.ini [Text_<LANG>]: LT_ key -> text. Unknown keys come back as the
// key itself, like the original shows them.
class Texts {
 public:
  void AddLayer(const std::string& path) { ini_.AddLayer(path); }
  void SetLanguage(const std::string& lang) { section_ = "Text_" + lang; }
  std::string operator()(const std::string& key) const;
  // A caption from Forms.ini: an LT_ key, or literal text.
  std::string Caption(const std::string& value) const;

 private:
  davi::LayeredIni ini_;
  std::string section_ = "Text_NL";
};

// Splits "a, b ,c" on commas and trims each part.
std::vector<std::string> Fields(const std::string& s);

}  // namespace er
