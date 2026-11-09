// High score tables (MdlScores in GameMenu.dat; re/menu/highscore_spec.md).
// The original keeps them in GameMenu.HS; the port in menu_scores.ini next
// to menu_players.ini. Without that file the GameMenu.ini [SCORE_*]
// defaults are used, as the original does without GameMenu.HS.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "engine/ini.h"

namespace er {

struct GameData;

// Adventure / championship row (0x28 bytes).
struct ScoreRow {
  std::string name;  // at most 8 characters
  int score = 0;
  int position = 0;  // final rank, 0-based (shown + 1)
  int club = 0;
  std::string date;  // "YYYY-MM-DD"
  uint32_t time = 0;  // total time, ms
};

// Lap or track record slot (0x28 bytes).
struct RecordSlot {
  int track = 0;
  std::string name;
  int car_type = 0;  // 0 standard / 1 ATV / 2 sport
  std::string date;
  uint32_t time = 0xffffffff;
};

struct ScoreDb {
  // 0 adventure, 1 standard, 2 ATV, 3 sport championship (high score tab order).
  std::vector<ScoreRow> tables[4];
  // [0] lap, [1] track records; per region 0 west / 1 middle / 2 south.
  std::vector<RecordSlot> records[2][3];

  // GameMenu.ini [SCORE_*] (loader 0x495c40): club Random(7) and total
  // time 0x05265818 for the rows, car type Random(3) and 3599990 ms for the
  // records, at most 9 records per region.
  void LoadDefaults(const davi::LayeredIni& ini, const GameData& data);
  bool Load(const std::string& path);
  bool Save(const std::string& path) const;
  // 0x495f74 and co: the row joins the table if it sorts into the top 5
  // (comparator 0x495668). 0 = not in, 1 = in, 2 = new top.
  int Insert(int table, const ScoreRow& row);
  // 0x4965dc / 0x496700: replaces the slot of that track if the new time is
  // strictly lower; true if it did. A track without a slot gets no record.
  bool Record(int kind, int region, const RecordSlot& slot);
};

}  // namespace er
