// game.ini before a race: what the menu tells the game (TGameInterfaceModule
// writer 0x4981c4 in GameMenu.dat). Key by key in re/menu/gameini_spec.md.
//
// Written: [PLAYER], [ETAPPE], [UPGRADES], [OPTIONS] race keys, [RACERS],
// [BONUS], [GameSim] Qualify, [InputGeneral] device, Resolution,
// [GFX OPTIONS], [Sound] from the options. Not yet: [InputKeyBinds],
// per-device sensitivity/force feedback, [DIRECTX] (template values stay).
#pragma once

#include "engine/ini.h"

namespace er {

class Game;

// TRaceType (RTTI 0x4a279a).
enum RaceType { kQrTrack, kQrDuel, kQrTimeTrial, kQrSuddenDeath, kQrNormal, kQrChase };

// Race type of the current race: the Quick Race choice, or the
// [CHAMPIONSHIP]/[ADVENTURE] row's Racetype (unknown names count as NORMAL).
int CurrentRaceType(Game& g);

// Sets every key above in `ini` (the live game.ini, other keys untouched).
void WriteGameIni(Game& g, davi::IniFile& ini);

}  // namespace er
