#include "er/league.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

namespace er {

void PositionReward(int position, int* points, int* money) {
  // DATA 0x4c8750 (points) and 0x4c8764 (money), places 1-5.
  static const int kPoints[5] = {30, 25, 15, 0, 0};
  static const int kMoney[5] = {5000, 2500, 1000, 0, 0};
  bool in = position >= 0 && position < 5;
  *points = in ? kPoints[position] : 0;
  *money = in ? kMoney[position] : 0;
}

void League::Reset() {
  int club = player_club;
  *this = League{};
  player_club = club;
}

void League::PickOpponents() {
  // Random(8) and, if taken, the next free club upwards (wrapping).
  bool taken[8] = {};
  if (player_club >= 0 && player_club < 8) taken[player_club] = true;
  for (int i = 0; i < 4; i++) {
    int c = std::rand() % 8;
    for (int k = 0; k < 8 && taken[c]; k++) c = (c + 1) % 8;
    taken[c] = true;
    opponents[i] = c;
  }
}

void League::AddResult(const RaceResult& r, int car_class) {
  int pts, money;
  ClubStats& me = clubs[player_club];
  PositionReward(r.position, &pts, &money);
  me.score += pts;
  me.money += money + r.money;
  me.time += r.tracktime;
  me.turbo = r.turbo;
  if (car_class >= 0 && car_class < 4) {
    me.damage[car_class] = r.damage;
    me.upgrade[car_class] = r.upgrades;
  }
  for (auto& o : r.racers) {
    if (o.club < 0 || o.club >= 8) continue;
    PositionReward(o.position, &pts, &money);
    clubs[o.club].score += pts;
    clubs[o.club].time += o.time;
  }
}

void League::Rank() {
  std::vector<int> order{player_club};
  for (int c : opponents) order.push_back(c);
  // TList.Sort (quicksort) on score; ties keep the list order here, the
  // original's order for ties is not defined.
  std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return clubs[a].score > clubs[b].score; });
  for (size_t i = 0; i < order.size(); i++) clubs[order[i]].rank = int(i);
}

int League::Leader() const {
  if (clubs[player_club].rank == 0) return player_club;
  for (int c : opponents)
    if (clubs[c].rank == 0) return c;
  return player_club;
}

bool League::NextRound(int races) {
  if (round + 1 >= races) return false;
  round++;
  return true;
}

}  // namespace er
