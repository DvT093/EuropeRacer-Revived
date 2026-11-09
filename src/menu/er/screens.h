// Shared pieces of the Europe Racer screens.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "er/game.h"

namespace er {

// Three polaroids (previous, current, next) with previous/next buttons, a
// name label and an info text (base class at GameMenu.dat 0x4b893c). An
// empty neighbour shows the "PolE" polaroid; the buttons are disabled at
// the ends.
struct Carousel {
  struct Item {
    int id = 0;
    std::string image, name, info;
  };
  explicit Carousel(ErScreen& s) : screen(s) {}
  ErScreen& screen;
  std::string prev, cur, next, name, info, btn_prev, btn_next;  // component names ("" = none)
  std::vector<Item> items;
  int index = 0;

  void Select(int i);  // clamps and redraws
  int IndexOf(int id) const;  // 0 if absent
  int Current() const { return items.empty() ? -1 : items[index].id; }
  // Handles the previous/next buttons; true if `w` was one of them.
  bool Click(davi::Widget& w);
};

std::unique_ptr<davi::Screen> MakeQuickRaceScreen(Game& g, const std::string& key);
std::unique_ptr<davi::Screen> MakeChampionshipScreen(Game& g, const std::string& key);
std::unique_ptr<davi::Screen> MakeDialogScreen(Game& g, const std::string& key);

}  // namespace er
