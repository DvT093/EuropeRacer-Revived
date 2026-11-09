#include "er/scores.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>

#include "er/gamedata.h"

namespace er {

namespace {

const char* kTables[4] = {"SCORE_ADVENTURE", "SCORE_CC_STANDARD", "SCORE_CC_ATV", "SCORE_CC_SPORT"};
const char* kRecords[2] = {"SCORE_LAP", "SCORE_TRACK"};
const char* kRegions[3] = {"TC_WESTEUROPE", "TC_MIDDLEEUROPE", "TC_SOUTHEUROPE"};

std::string Today() {
  std::time_t t = std::time(nullptr);
  char b[16];
  std::strftime(b, sizeof b, "%Y-%m-%d", std::localtime(&t));
  return b;
}

// Comparator 0x495668: score high first, then position high first (as
// coded), time low first, date new first.
bool Before(const ScoreRow& a, const ScoreRow& b) {
  if (a.score != b.score) return a.score > b.score;
  if (a.position != b.position) return a.position > b.position;
  if (a.time != b.time) return a.time < b.time;
  return a.date > b.date;
}

}  // namespace

void ScoreDb::LoadDefaults(const davi::LayeredIni& ini, const GameData& data) {
  for (int t = 0; t < 4; t++) {
    tables[t].clear();
    for (int i = 0; i < 5; i++) {
      std::vector<std::string> f = Fields(ini.Get(kTables[t], std::to_string(i)));
      ScoreRow r;
      if (!f.empty()) r.name = f[0].substr(0, 8);
      if (f.size() > 1) r.score = std::atoi(f[1].c_str());
      if (f.size() > 2) r.position = std::atoi(f[2].c_str());
      r.club = std::rand() % 7;
      r.time = 0x05265818;
      r.date = Today();
      tables[t].push_back(r);
    }
  }
  for (int k = 0; k < 2; k++) {
    for (auto& reg : records[k]) reg.clear();
    int n = ini.GetInt(kRecords[k], "Count");
    for (int i = 0; i < n; i++) {
      std::vector<std::string> f = Fields(ini.Get(kRecords[k], std::to_string(i)));
      if (f.size() < 2) continue;
      RecordSlot s;
      s.track = std::atoi(f[0].c_str());
      s.name = f[1].substr(0, 8);
      s.car_type = std::rand() % 3;
      s.time = 3599990;
      s.date = Today();
      const Track* tr = data.TrackById(s.track);
      int reg = 0;
      while (tr && reg < 3 && tr->region != kRegions[reg]) reg++;
      if (reg >= 3 || records[k][reg].size() >= 9) continue;  // 10th: ERangeError in the original
      records[k][reg].push_back(s);
    }
  }
}

bool ScoreDb::Load(const std::string& path) {
  davi::IniFile ini;
  if (!ini.Load(path)) return false;
  for (int t = 0; t < 4; t++) {
    tables[t].clear();
    for (int i = 0; i < 5; i++) {
      std::vector<std::string> f = Fields(ini.Get(kTables[t], std::to_string(i)));
      ScoreRow r;
      if (f.size() >= 6) {
        r.name = f[0], r.score = std::atoi(f[1].c_str()), r.position = std::atoi(f[2].c_str());
        r.club = std::atoi(f[3].c_str()), r.date = f[4], r.time = uint32_t(std::strtoul(f[5].c_str(), nullptr, 10));
      }
      tables[t].push_back(r);
    }
  }
  for (int k = 0; k < 2; k++)
    for (int reg = 0; reg < 3; reg++) {
      records[k][reg].clear();
      std::string sec = std::string(kRecords[k]) + "." + kRegions[reg];
      int n = ini.GetInt(sec, "Count");
      for (int i = 0; i < n && i < 9; i++) {
        std::vector<std::string> f = Fields(ini.Get(sec, std::to_string(i)));
        if (f.size() < 5) continue;
        RecordSlot s;
        s.track = std::atoi(f[0].c_str()), s.name = f[1], s.car_type = std::atoi(f[2].c_str());
        s.date = f[3], s.time = uint32_t(std::strtoul(f[4].c_str(), nullptr, 10));
        records[k][reg].push_back(s);
      }
    }
  return true;
}

bool ScoreDb::Save(const std::string& path) const {
  davi::IniFile ini;
  for (int t = 0; t < 4; t++)
    for (size_t i = 0; i < tables[t].size(); i++) {
      const ScoreRow& r = tables[t][i];
      ini.Set(kTables[t], std::to_string(i),
              r.name + "," + std::to_string(r.score) + "," + std::to_string(r.position) + "," + std::to_string(r.club) +
                  "," + r.date + "," + std::to_string(r.time));
    }
  for (int k = 0; k < 2; k++)
    for (int reg = 0; reg < 3; reg++) {
      std::string sec = std::string(kRecords[k]) + "." + kRegions[reg];
      ini.Set(sec, "Count", std::to_string(records[k][reg].size()));
      for (size_t i = 0; i < records[k][reg].size(); i++) {
        const RecordSlot& s = records[k][reg][i];
        ini.Set(sec, std::to_string(i),
                std::to_string(s.track) + "," + s.name + "," + std::to_string(s.car_type) + "," + s.date + "," +
                    std::to_string(s.time));
      }
    }
  return ini.Save(path);
}

int ScoreDb::Insert(int table, const ScoreRow& row) {
  if (table < 0 || table > 3) return 0;
  std::vector<ScoreRow> all = tables[table];
  all.push_back(row);
  // Index of the new row after the sort decides the result.
  std::vector<size_t> order(all.size());
  for (size_t i = 0; i < order.size(); i++) order[i] = i;
  std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return Before(all[a], all[b]); });
  size_t pos = std::find(order.begin(), order.end(), all.size() - 1) - order.begin();
  if (pos >= 5) return 0;
  std::vector<ScoreRow> out;
  for (size_t i = 0; i < 5 && i < order.size(); i++) out.push_back(all[order[i]]);
  tables[table] = out;
  return pos == 0 ? 2 : 1;
}

bool ScoreDb::Record(int kind, int region, const RecordSlot& slot) {
  if (kind < 0 || kind > 1 || region < 0 || region > 2) return false;
  bool any = false;
  for (auto& s : records[kind][region])
    if (s.track == slot.track && s.time > slot.time) s = slot, any = true;
  return any;
}

}  // namespace er
