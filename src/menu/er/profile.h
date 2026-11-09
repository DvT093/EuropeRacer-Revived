// Players and settings between sessions. The original keeps them in
// GameMenu.EUR, a raw dump of its player object (load 0x4a30e4, save
// 0x4a3244; layout in re/menu/players_options_spec.md): five profiles and
// the global display/sound options. The port keeps the same data as a
// readable, editable ini in SwARc's per-game data dir (menu_players.ini).
#pragma once

#include <string>

#include "er/league.h"

namespace er {

// Global options (player object +0x3764..+0x379c).
struct GlobalOptions {
  int dx_index = 0;      // +0x3764 [DIRECTX] Index
  int resolution = 0;    // +0x3768 0 low / 1 medium / 2 high -> Resolution {3,4,7}
  int brightness = 0;    // +0x376c 0 light / 1 dark -> gamma
  int mipmapping = 0;    // +0x3770
  int texture_detail = 0;  // +0x3774 (not written to game.ini)
  int filter = 1;        // +0x3778 bilinear -> FILTER 2/0
  int transparency = 1;  // +0x377c -> alpha_blending
  int wait_vbl = 0;      // +0x3784
  int speakers = 0;      // +0x3788 two / four / headphones -> [Sound] hardware
  int sound_system = 0;  // +0x378c normal / EAX / A3D -> [Sound] system
  int sound_quality = 0;  // +0x3790
  int sound_on = 1;      // +0x3794
  int hide_particles = 0;    // +0x3798 -> particles = 1 - v
  int hide_pedestrians = 0;  // +0x379c -> Pedestrians 0
};

// One player (profile record, 0xb10 bytes in the original).
struct Profile {
  bool used = false;       // R+0 (the original stores "empty")
  std::string name;        // R+1, at most 8 characters
  std::string date;        // R+0x10 entry date, "YYYY-MM-DD"
  int game_type = 0;       // R+0xb0c 0 QR / 1 CH / 2 AD
  int ch_class = 0;        // R+0x7cc current championship class
  League ch[3], ad;        // R+0x1c (per class) / R+0x7d0
  // Quick Race choices (R+0x9c4..).
  int qr_type = 0, qr_difficulty = 1, qr_track = -1, qr_car = -1, qr_opponent = 0;
  int ch_difficulty = 1, club = 0, ad_car = -1;
  int input_device = 0;    // R+0x9f4 keyboard / gamepad / joystick / wheel
  int show_controls = 1;   // R+0xafc
  int music = 50, sfx = 50, voices = 50;  // R+0xb00..0xb08
};

struct PlayerDb {
  Profile players[5];
  int current = 0;  // +0x3760
  GlobalOptions options;
  bool Load(const std::string& path);  // false: no file (defaults kept)
  bool Save(const std::string& path) const;
  bool Empty() const;  // 0x4a48dc: every slot free
  Profile& Current() { return players[current < 0 || current > 4 ? 0 : current]; }
};

}  // namespace er
