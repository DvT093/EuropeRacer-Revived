// The race's answer: what the menu reads back from game.ini after a race
// (MdlGameInterface.pas in GameMenu.dat) and where the flow goes next.
//
// The original menu quit before the race; the launcher (ER.dat) ran the
// game and then restarted the menu with /DEFAULT, which read game.ini
// (0x497b18) and picked the next flow state (0x494218).
#pragma once

#include <string>
#include <vector>

#include "engine/ini.h"

namespace er {

// One opponent of [RACERS]: "<i>=car,position,time" plus "country<ii>" (the
// club the racer drives for, which is also its picture ChaW<club>).
struct RacerResult {
  int club = 0;
  int position = 0;  // zero based: 0 = won
  int time = 0;
};

// TGameInterfaceModule's import record (0x497c54, 0x497ea8).
struct RaceResult {
  bool imported = false;
  bool finished = false;  // [PLAYER] Finished
  int position = -1;      // [PLAYER] Position, zero based
  int tracktime = 0;      // [PLAYER] TrackTime (ms)
  int best_lap = 0x7fffffff;  // the lowest of lap_1_time..lap_<lapcount>_time
  int turbo = 0, damage = 0, money = 0, upgrades = 0;
  std::vector<RacerResult> racers;  // [OPTIONS] NrOpponents of them (max 4)

  bool Load(const davi::IniFile& ini);
  // 0x49a09c: 0 = the player won, i + 1 = racers[i] won, -1 = nobody
  // ("No winner!").
  int Winner() const;
};

}  // namespace er
