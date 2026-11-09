// eracer-menu: native replacement for Europe Racer's Delphi menu
// (Menu/GameMenu.dat). Forms, pictures, fonts, texts and the flow are read
// at run time from the user's own CD.
//
//   eracer-menu --game-dir DIR [--lang NL] [--mods DIR] [--script FILE] [--shot FILE]
//
// DIR is the CD's EuropeRacer folder (ER.dat, Menu/, Game/); the language
// data is read from ../Langdata/<lang>. --script runs scripted input for
// tests (see App::LoadScript); F12 saves a screenshot to --shot.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "engine/app.h"
#include "er/game.h"

int main(int argc, char** argv) {
  std::string game_dir, lang = "NL", mods_dir, script, shot;
  if (const char* e = std::getenv("ERACER_GAME_DIR")) game_dir = e;
  for (int i = 1; i < argc; i++) {
    auto next = [&](std::string* out) {
      if (i + 1 < argc) *out = argv[++i];
    };
    if (!std::strcmp(argv[i], "--game-dir")) next(&game_dir);
    else if (!std::strcmp(argv[i], "--lang")) next(&lang);
    else if (!std::strcmp(argv[i], "--mods")) next(&mods_dir);
    else if (!std::strcmp(argv[i], "--script")) next(&script);
    else if (!std::strcmp(argv[i], "--shot")) next(&shot);
    else {
      std::fprintf(stderr, "usage: %s --game-dir DIR [--lang NL] [--mods DIR] [--script FILE] [--shot FILE]\n", argv[0]);
      return 2;
    }
  }
  if (game_dir.empty()) {
    std::fprintf(stderr, "eracer-menu: pass --game-dir or set ERACER_GAME_DIR\n");
    return 2;
  }
  while (game_dir.size() > 1 && game_dir.back() == '/') game_dir.pop_back();
  std::string menu_dir = davi::FindNoCase(game_dir, "Menu");
  std::string lang_dir = davi::FindNoCase(game_dir, "../Langdata/" + lang);

  davi::DelphiExe exe;
  std::string menu_exe = davi::FindNoCase(menu_dir, "GameMenu.dat");
  if (menu_exe.empty() || !exe.Load(menu_exe)) {
    std::fprintf(stderr, "eracer-menu: cannot read %s/Menu/GameMenu.dat\n", game_dir.c_str());
    return 1;
  }
  davi::Assets assets;
  if (!mods_dir.empty()) assets.AddDir(mods_dir);
  assets.AddDir(menu_dir);
  if (!lang_dir.empty()) assets.AddDir(lang_dir + "/menu");
  assets.AddArchive(menu_dir, "Graphics");
  assets.AddArchive(menu_dir, "Music");
  if (!lang_dir.empty()) assets.AddArchive(lang_dir + "/menu", "Voices");

  er::Texts texts;
  texts.SetLanguage(lang);
  texts.AddLayer(davi::FindNoCase(lang_dir, "menu/Language.ini"));
  if (!mods_dir.empty()) texts.AddLayer(mods_dir + "/Language.ini");

  davi::Skin skin;
  skin.exe = &exe;
  skin.assets = &assets;
  // Layout: Forms.ini + FormsA.ini (the second half of the forms), mod on top.
  skin.caps.AddLayer(davi::FindNoCase(menu_dir, "Forms.ini"));
  skin.caps.AddLayer(davi::FindNoCase(menu_dir, "FormsA.ini"));
  if (!mods_dir.empty()) skin.caps.AddLayer(mods_dir + "/Forms.ini");
  // Fonts: "Hilde" is installed from the CD (FONTDATA/HIS_____.TTF); the
  // rest (MS Sans Serif) falls back to Arial.
  auto hilde = std::make_unique<davi::Font>();
  std::string hilde_path = davi::FindNoCase(game_dir, "../FONTDATA/HIS_____.TTF");
  if (!hilde_path.empty() && hilde->LoadFile(hilde_path)) skin.fonts[1] = std::move(hilde);
  auto arial = std::make_unique<davi::Font>();
  if (arial->LoadFile("/System/Library/Fonts/Supplemental/Arial.ttf")) skin.fonts[2] = std::move(arial);
  auto hilde_bold = std::make_unique<davi::Font>();
  if (!hilde_path.empty() && hilde_bold->LoadFile(hilde_path)) {
    hilde_bold->embolden = true;  // the CD only has the regular face
    skin.fonts[3] = std::move(hilde_bold);
  }
  auto arial_bold = std::make_unique<davi::Font>();
  if (arial_bold->LoadFile("/System/Library/Fonts/Supplemental/Arial Bold.ttf")) skin.fonts[4] = std::move(arial_bold);
  skin.font_names = {{"hilde", 1}, {"hilde bold", 3}, {"ms sans serif", 2}, {"arial", 2}, {"arial bold", 4},
                     {"ms sans serif bold", 4}};
  er::RegisterSkin(skin, texts);

  davi::App app(skin);
  davi::AppConfig cfg;
  cfg.title = texts("LT_APPTITLE");
  cfg.screenshot_path = shot;
  // Tests: ERACER_MENU_UPGRADES=0 draws the original 800x600 look,
  // ERACER_MENU_SIZE=WxH renders offscreen at that size (as Vakantie Racer).
  if (const char* u = std::getenv("ERACER_MENU_UPGRADES")) cfg.upgraded = *u != '0';
  if (const char* sz = std::getenv("ERACER_MENU_SIZE")) std::sscanf(sz, "%dx%d", &cfg.render_w, &cfg.render_h);
  if (!app.Init(cfg)) {
    std::fprintf(stderr, "eracer-menu: cannot open the window: %s\n", SDL_GetError());
    return 1;
  }

  er::Game game(app);
  if (!lang_dir.empty()) game.media_dir = lang_dir + "/Media";
  game.install = {davi::FindNoCase(game_dir, "Game"), red::Install::DefaultUserDir("eracer")};
  game.texts = texts;
  game.gamemenu.AddLayer(davi::FindNoCase(menu_dir, "GameMenu.ini"));
  if (!mods_dir.empty()) game.gamemenu.AddLayer(mods_dir + "/GameMenu.ini");
  if (!game.data.Load(game.gamemenu) || !game.flow.Load(game.gamemenu)) {
    std::fprintf(stderr, "eracer-menu: GameMenu.ini has no tracks or flow\n");
    return 1;
  }
  app.on_button_down = [](davi::Widget&) {};
  if (!script.empty()) app.LoadScript(script);
  game.PlayMusic("SNDCB_NORMAL");
  game.Go("FS_DEFAULT");
  return app.Run();
}
