#include "er/table.h"

#include <cstdio>

#include "engine/dfm.h"
#include "engine/form.h"
#include "engine/text.h"
#include "er/gamedata.h"

namespace er {

void ScoreTable::Load(const davi::LayeredIni& ini, const Texts& texts, const std::string& sec,
                      std::initializer_list<const char*> names) {
  y = ini.GetInt(sec, "CTT_Y");
  title_h = ini.GetInt(sec, "CTT_TITLEHEIGHT");
  row_h = ini.GetInt(sec, "CTT_CONTROLHEIGHT");
  title_px = -ini.GetInt(sec, "CTT_TITLEFONTHEIGHT");
  row_px = -ini.GetInt(sec, "CTT_CONTROLFONTHEIGHT");
  cols.clear();
  for (const char* c : names)
    cols.push_back({ini.GetInt(sec, std::string("CTT_") + c + "_X"),
                    texts(davi::Trim(ini.Get(sec, std::string("CTT_") + c + "_TITLE")))});
}

void ScoreTable::Draw(davi::Skin& skin, SDL_Surface* canvas, int x, int y0,
                      const std::vector<std::vector<std::string>>& rows) const {
  davi::Font* f = skin.GetFont(1);  // Hilde
  if (!f) return;
  auto cell = [&](int cx, int cy, const std::string& s, int px) {
    davi::TextStyle st{f, px, {0, 0, 0, 255}, davi::HAlign::kLeft, davi::VAlign::kTop, false};
    davi::DrawText(canvas, SDL_Rect{cx, cy, 800, px * 2}, s, st);
  };
  for (auto& [cx, title] : cols) cell(x + cx, y0 + y, title, title_px);
  for (size_t r = 0; r < rows.size(); r++)
    for (size_t c = 0; c < rows[r].size() && c < cols.size(); c++)
      cell(x + cols[c].first, y0 + y + title_h + int(r) * row_h, rows[r][c], row_px);
}

std::string Hms(int ms) {
  char b[32];
  int s = ms / 1000;
  std::snprintf(b, sizeof b, "%.2d:%.2d:%.2d", s / 3600 % 60, s / 60 % 60, s % 60);
  return b;
}

std::string Msh(int ms) {
  char b[32];
  int s = ms / 1000;
  std::snprintf(b, sizeof b, "%.2d:%.2d:%.2d", s / 60 % 60, s % 60, ms % 1000 / 10);
  return b;
}

}  // namespace er
