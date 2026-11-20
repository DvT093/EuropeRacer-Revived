#include "er/race.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "er/gamedata.h"

namespace er {

// TGameInterfaceModule.ImportPlayer 0x497c54 and ImportRacers 0x497ea8.
bool RaceResult::Load(const davi::IniFile& ini) {
  *this = RaceResult{};
  if (!ini.Loaded()) return false;
  finished = ini.GetInt("PLAYER", "Finished", 0) != 0;
  position = ini.GetInt("PLAYER", "Position", -1);
  tracktime = ini.GetInt("PLAYER", "TrackTime", 0);
  out_of_time = ini.GetInt("PLAYER", "out_of_time", 0) != 0;
  int laps = ini.GetInt("PLAYER", "lapcount", 0);
  for (int i = 1; i <= laps; i++) {
    char k[32];
    std::snprintf(k, sizeof k, "lap_%d_time", i);
    best_lap = std::min(best_lap, ini.GetInt("PLAYER", k, 0x7fffffff));
  }
  damage = ini.GetInt("PLAYER", "damage", 0);
  turbo = ini.GetInt("PLAYER", "TURBO_AMOUNT", 0);
  money = ini.GetInt("PLAYER", "Money", 0);
  upgrades = ini.GetInt("UPGRADES", "PLAYER", 0);
  int n = ini.GetInt("OPTIONS", "NrOpponents", 0);
  if (n > 4) {
    SDL_Log("menu: There are too many racers to show on the result list! (%d)", n);
    n = 4;
  }
  for (int i = 0; i < n; i++) {
    std::string key = std::to_string(i);
    if (!ini.Has("RACERS", key)) {
      SDL_Log("menu: Error in interface file, key SECTION_RACERS. %d does not exist", i);
      continue;
    }
    // "car,position,time": the car the menu wrote is skipped.
    std::vector<std::string> f = Fields(ini.Get("RACERS", key));
    RacerResult r;
    if (f.size() > 1) r.position = std::atoi(f[1].c_str());
    if (f.size() > 2) r.time = std::atoi(f[2].c_str());
    char ck[32];
    std::snprintf(ck, sizeof ck, "country%.2d", i);
    r.club = ini.GetInt("RACERS", ck, 0);
    racers.push_back(r);
  }
  imported = true;
  return true;
}

int RaceResult::Winner() const {
  if (position == 0) return 0;
  for (size_t i = 0; i < racers.size(); i++)
    if (racers[i].position == 0) return int(i) + 1;
  SDL_Log("menu: No winner!");
  return -1;
}

}  // namespace er
