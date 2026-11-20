// eracer: Europe Racer (Davilex, 2001), natively.
//
//   eracer [--game-dir DIR] [--exe FILE] [--lang NL] [-- GAME ARGS...]
//
// DIR is the game's folder from the user's own CD (EuropeRacer/Game: Cars*,
// Etappe*, RC.IMG, Ini/game.ini). Defaults to $ERACER_GAME_DIR. FILE is the
// official NoCD spel.exe (Davilex, 15-06-2002; dl/nocd/spel.exe in the
// project): the CD's own exe is SafeDisc-protected. Set ERACER_TRACE=1 to
// log every HLE call.

#include <sys/stat.h>

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <filesystem>
#include <string>

#include <SDL3/SDL.h>

#include "runtime/log.h"
#include "win32/process.h"

namespace eracer {
extern int g_render_w, g_render_h;  // game/gfx_resolution.cpp
void ScreenSize(int* w, int* h);
}  // namespace eracer

namespace {

namespace fs = std::filesystem;

// SwARc's per-game data dir: $XDG_DATA_HOME/<id>, else Application Support.
std::string DataDir(const std::string& id) {
  if (const char* x = std::getenv("XDG_DATA_HOME"); x && *x) return std::string(x) + "/" + id;
  const char* home = std::getenv("HOME");
  return std::string(home ? home : ".") + "/Library/Application Support/" + id;
}

// The installer copied one language's Langdata/<lang>/Game (RcsLang.*,
// ini/events.ini) over the game folder. Natively the CD stays read-only:
// the files go into SwARc's overlay (<data dir>/install), which the game
// sees in its own folder.
void InstallLanguage(const std::string& game_dir, const std::string& lang, const std::string& overlay) {
  fs::path src = fs::path(game_dir).parent_path().parent_path() / "Langdata" / lang / "Game";
  std::error_code ec;
  if (!fs::is_directory(src, ec)) {
    std::fprintf(stderr, "eracer: no language data in %s\n", src.c_str());
    return;
  }
  for (auto it = fs::recursive_directory_iterator(src, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
    if (!it->is_regular_file(ec) || it->path().extension() == ".txt") continue;
    fs::path dst = fs::path(overlay) / fs::relative(it->path(), src, ec);
    if (fs::exists(dst, ec) && fs::file_size(dst, ec) == it->file_size(ec)) continue;
    fs::create_directories(dst.parent_path(), ec);
    fs::copy_file(it->path(), dst, fs::copy_options::overwrite_existing, ec);
  }
  // Ini/language.ini LANGUAGE (0 Dutch, 1 English, 2 German, 3 French,
  // 4 Spanish, 5 Italian): the CD has 4; the installer wrote the chosen one.
  static const char* kCodes[] = {"NL", "UK", "DE", "FR", "ES", "IT"};
  int idx = 0;
  for (int i = 0; i < 6; i++)
    if (!strcasecmp(lang.c_str(), kCodes[i])) idx = i;
  fs::path ini_src = fs::path(game_dir) / "Ini" / "language.ini";
  fs::path ini_dst = fs::path(overlay) / "Ini" / "language.ini";
  if (!fs::exists(ini_dst, ec)) {
    fs::create_directories(ini_dst.parent_path(), ec);
    fs::copy_file(ini_src, ini_dst, ec);
  }
  std::FILE* f = std::fopen(ini_dst.c_str(), "rb");
  if (!f) return;
  std::string text;
  char buf[4096];
  for (size_t n; (n = std::fread(buf, 1, sizeof buf, f)) > 0;) text.append(buf, n);
  std::fclose(f);
  size_t at = text.find("LANGUAGE=");
  if (at == std::string::npos) return;
  size_t end = text.find_first_of("\r\n", at);
  std::string want = "LANGUAGE=" + std::to_string(idx);
  if (text.compare(at, end - at, want) == 0) return;
  text.replace(at, end - at, want);
  if ((f = std::fopen(ini_dst.c_str(), "wb"))) std::fwrite(text.data(), 1, text.size(), f), std::fclose(f);
}

}  // namespace

int main(int argc, char** argv) {
  std::string game_dir, exe, lang = "NL", game_args;
  if (const char* e = std::getenv("ERACER_GAME_DIR")) game_dir = e;
  for (int i = 1; i < argc; i++) {
    if (!std::strcmp(argv[i], "--game-dir") && i + 1 < argc) {
      game_dir = argv[++i];
    } else if (!std::strcmp(argv[i], "--exe") && i + 1 < argc) {
      exe = argv[++i];
    } else if (!std::strcmp(argv[i], "--lang") && i + 1 < argc) {
      lang = argv[++i];
    } else if (!std::strcmp(argv[i], "--")) {
      for (i++; i < argc; i++) {
        if (!game_args.empty()) game_args += ' ';
        game_args += argv[i];
      }
    } else {
      std::fprintf(stderr, "usage: %s [--game-dir DIR] [--exe FILE] [--lang NL] [-- GAME ARGS...]\n", argv[0]);
      return 2;
    }
  }
  if (game_dir.empty()) {
    std::fprintf(stderr, "eracer: pass --game-dir or set ERACER_GAME_DIR\n");
    return 2;
  }
  while (game_dir.size() > 1 && game_dir.back() == '/') game_dir.pop_back();
  if (const char* t = std::getenv("ERACER_TRACE")) recomp::g_trace = *t && *t != '0';

  // The project root: <project>/out/build/release/eracer.
  std::string root;
  {
    char self[PATH_MAX];
    if (realpath(argv[0], self)) {
      root = self;
      for (int up = 0; up < 4; up++) root = root.substr(0, root.find_last_of('/'));
    }
  }
  if (exe.empty()) exe = root + "/dl/nocd/spel.exe";
  struct stat st;
  if (stat(exe.c_str(), &st) != 0) {
    std::fprintf(stderr, "eracer: %s not found (the NoCD spel.exe, see CLAUDE.md)\n", exe.c_str());
    return 2;
  }

  recomp::win32::GameConfig game;
  game.id = "eracer";
  game.guest_dir = "C:\\Europe Racer\\Game";
  game.exe_name = "spel.exe";
  // The race overflowed Windows' 1 MiB main stack after about a minute
  // (20 nov 2026, AI waypoint search, right after the window changed
  // Spaces): give it room.
  game.main_stack_size = 100ull << 20;
  // The disc (volume label ER) is the folder above EuropeRacer/.
  std::string cd_dir = fs::path(game_dir).parent_path().parent_path().string();
  game.cd_drives = {{'D', cd_dir, "ER"}};
  InstallLanguage(game_dir, lang, DataDir(game.id) + "/install");
  // Same engine family as Vakantie Racer: 60 fps as on the hardware of its
  // day; ERACER_MAX_FPS=0 uncaps (SwARc). Check FPSMULT before relying on it.
  game.max_fps = 60;

  // Image upgrades as in Vakantie Racer and A2 Racer IV: render at the
  // screen's own 4:3 size (src/game/gfx_resolution.cpp,
  // ERACER_RESOLUTION=original|WxH overrides), MSAA with
  // alpha-to-coverage, mipmaps with trilinear and 16x anisotropic
  // filtering, HD textures. F9 switches all of it between original and
  // upgraded, F10 only anti-aliasing, F11 only filtering.
  eracer::ScreenSize(&eracer::g_render_w, &eracer::g_render_h);
  if (const char* r = std::getenv("ERACER_RESOLUTION")) {
    int w = 0, h = 0;
    if (!std::strcmp(r, "original")) eracer::g_render_w = eracer::g_render_h = -1;
    else if (std::sscanf(r, "%dx%d", &w, &h) == 2 && w >= 320 && h >= 240) eracer::g_render_w = w, eracer::g_render_h = h;
  }
  game.gfx.msaa = 8;
  game.gfx.alpha_to_coverage = true;
  game.gfx.mipmaps = true;
  game.gfx.anisotropy = 16;
  game.gfx_enhanced = true;
  // Turbo on Cmd: macOS keeps Ctrl+arrow for itself (Spaces), so the race
  // sees the Command keys as Ctrl and the real Ctrl keys not at all.
  game.key_aliases = {{SDL_SCANCODE_LGUI, SDL_SCANCODE_LCTRL},
                      {SDL_SCANCODE_RGUI, SDL_SCANCODE_RCTRL},
                      {SDL_SCANCODE_LCTRL, 0},
                      {SDL_SCANCODE_RCTRL, 0}};
  game.gfx_key_all = SDL_SCANCODE_F9;
  game.gfx_key_aa = SDL_SCANCODE_F10;
  game.gfx_key_filter = SDL_SCANCODE_F11;
  game.gfx_label_original = "Origineel";
  game.gfx_label_enhanced = "Upgraded";
  // HD textures (tools/make_texture_pack.py, from your own CD): SwARc knows
  // each texture by its pixels and draws the 4x version while Upgraded is
  // on (F9). ERACER_TEX_PACK=dir overrides the place.
  game.gfx.replace_textures = true;
  if (!root.empty() && stat((root + "/build/texture_pack").c_str(), &st) == 0) game.texture_pack = root + "/build/texture_pack";
  return recomp::RunProcess(game, exe, game_dir, game_args);
}
