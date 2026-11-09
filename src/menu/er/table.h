// Score tables drawn by the menu's own controls (TCtrlScore family,
// 0x4846a8): a title row and rows of cells in Hilde, black, at the columns
// of a [SCORE_LAYOUT-x] section of GameMenu.ini.
//
//   A  high score Adventure/Championship  NAME SCORE POSITION CLUB TIME
//   B  race result (ControlGameResults)   NAME TRACKTIME
//   C  standings (ControlEndRanking)      NAME CLUB SCORE TIME
//   D  status (ControlStatusRanking)      NAME CLUB SCORE
//   E  high score lap/track               NAME TRACK CARTYPE TRACKTIME
#pragma once

#include <SDL3/SDL.h>

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "engine/ini.h"

namespace davi {
class Skin;
}

namespace er {

class Texts;

struct ScoreTable {
  int y = 0, title_h = 0, row_h = 0, title_px = 0, row_px = 0;
  std::vector<std::pair<int, std::string>> cols;  // x, title text

  // CTT_Y, CTT_TITLEHEIGHT, CTT_CONTROLHEIGHT, -CTT_TITLEFONTHEIGHT,
  // -CTT_CONTROLFONTHEIGHT and per column CTT_<col>_X / CTT_<col>_TITLE.
  void Load(const davi::LayeredIni& ini, const Texts& texts, const std::string& sec,
            std::initializer_list<const char*> cols);
  // Titles at y + CTT_Y, rows below them. Uses davi::DrawText, so on the
  // PaintOver canvas the text is drawn sharp at the window's resolution.
  void Draw(davi::Skin& skin, SDL_Surface* canvas, int x, int y,
            const std::vector<std::vector<std::string>>& rows) const;
};

std::string Hms(int ms);  // 0x468018: hh:mm:ss
std::string Msh(int ms);  // 0x4680c8: mm:ss:hundredths

}  // namespace er
