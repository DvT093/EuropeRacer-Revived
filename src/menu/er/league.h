// A championship (per car class) or the adventure: the player against four
// club racers, with points, money and total time per club. The original
// keeps one such record per class in the player's save (CH: player object
// +0x2c + class*0x1ec, AD: +0x7e0); offsets below are within that record.
#pragma once

#include "er/race.h"

namespace er {

struct ClubStats {
  int money = 0;   // +0x20
  int score = 0;   // +0x24
  int rank = 0;    // +0x28, 0 = leads (set by Rank)
  int time = 0;    // +0x2c, total race time (ms)
  int turbo = 100;  // +0x34
  int damage[4] = {0, 0, 0, 0};   // +0x00 + class*8
  int upgrade[4] = {0, 0, 0, 0};  // +0x04 + class*8
};

struct League {
  bool is_new = true;  // set up the opponents before the first race
  int player_club = 0;     // +0x1d4
  int opponents[4] = {0, 1, 2, 3};  // +0x1c0, clubs of the four racers
  int round = 0;           // CH +0x1e4, AD +0x1e8
  ClubStats clubs[8];

  // 0x4a2be8: everything back to the start (turbo 100).
  void Reset();
  // 0x4a2a50: four distinct random clubs, none the player's.
  void PickOpponents();
  // 0x4be724 (TFrmCHResultRanking create): points and money for the
  // positions (30/25/15 points, 5000/2500/1000), plus time; the player also
  // gets the race's Money, turbo, and damage/upgrades for the car class.
  void AddResult(const RaceResult& r, int car_class);
  // 0x4beb50: player and opponents by score, highest first.
  void Rank();
  // The club in front (rank 0).
  int Leader() const;
  // 0x4a5054: to the next race if there is one; false at the end.
  bool NextRound(int races);
};

// 0x4a517c: points and money for a zero-based position.
void PositionReward(int position, int* points, int* money);

}  // namespace er
