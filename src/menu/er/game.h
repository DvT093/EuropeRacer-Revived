// Europe Racer menu: shared state and the wizard flow. Every screen is a
// form of GameMenu.dat; which form follows which comes from the flow matrix
// in GameMenu.ini (see flow.h). Graphic names that the original sets in code
// are in skin.cpp, with the GameMenu.dat address they come from.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "engine/app.h"
#include "er/flow.h"
#include "er/gamedata.h"
#include "er/league.h"
#include "er/race.h"

namespace er {

class Game;

// One wizard/popup form. The base handles the buttons every TBaseFrmWizard /
// TBaseFrmPopUpWindow has (Next, Previous, Quit, Ok, Cancel); a form with
// its own logic derives from it.
class ErScreen : public davi::Screen {
 public:
  ErScreen(Game& g, const std::string& cls);
  Game& game;
  void OnClick(davi::Widget& w) override;
  bool OnKey(SDL_Keycode k) override;
  void OnShow() override { Layout(); }
  bool PreviousAllowed() const { return previous_allowed_; }
  // Button placement the original does in code on every show (virtual
  // 0xe4 of each form class); it overrides the Forms.ini coordinates.
  virtual void Layout();

 protected:
  // Radio group controller: radio i shows texts[i] in TxtInfo and, if given,
  // pictures[i] in the image component `image`.
  void BindRadios(std::vector<std::string> texts, std::vector<std::string> pictures, const std::string& image = "");
  void SetRadio(int i);    // checks BtnRadio<i+1>
  int CheckedRadio() const;  // 0-based, 0 if none
  // Shows or hides a panel together with everything inside it.
  void SetPanelVisible(const std::string& panel, bool v);

 public:
  void SetPanel(const std::string& panel, bool v) { SetPanelVisible(panel, v); }

 protected:
  // Fills "%s" in a message with the player's name.
  std::string Format(const std::string& msg) const;

 private:
  bool previous_allowed_ = true;  // BtnPrevious Visible in the DFM
  std::vector<std::string> radio_texts_, radio_pics_;
  std::string radio_image_;
  void ApplyRadio();
};

// Choices made along the flow (what the original keeps in its modules).
struct Choices {
  std::string game_type;  // "QR", "CH", "AD"
  int difficulty = 1;     // 0 easy, 1 normal, 2 hard
  int ch_type = 0;        // championship class: 0 standard, 1 ATV, 2 sport
  int qr_type = 0;        // 0 single track, 1 duel, 2 time trial, 3 sudden death
  int track = -1, car = -1, club = 0, opponent = 0;  // game ids (-1 = first in the list)
  std::string player = "Speler";  // until the player list exists
};

class Game {
 public:
  Game(davi::App& a) : app(a) {}
  davi::App& app;
  GameData data;
  FlowMatrix flow;
  Texts texts;
  davi::LayeredIni gamemenu;  // GameMenu.ini (+ mod layer)
  Choices choices;
  RaceResult race;  // the last race's result (game.ini)
  bool race_counted = false;  // race already added to the standings
  League ch[3], ad;  // championship per car class, adventure
  // The standings being played (CH: the chosen class; AD); nullptr for QR.
  League* CurrentLeague();
  // Races in the current league: CH 5 per class, AD all [ADVENTURE] rows.
  int LeagueRaces() const;
  std::string media_dir;  // Langdata/<lang>/Media (movies)

  // Shows the form of the current flow step.
  void ShowCurrent();
  // Wizard buttons (TBaseFrmWizard.BtnNextClick / BtnPreviousClick).
  // Both first switch the flow back to the game type's play state, keeping
  // the form (TFlowMatrix rebase 0x4941e4, from 0x492ad0 / 0x492a8c): a
  // result flow (FS_CHQUALIFIED ...) ends at ShowTrack, and Next there goes
  // on in FS_CHSTANDARD. The result forms step without it (NextPlain).
  void Next();
  void Previous();
  void NextPlain();
  // Player object +0x37a0: 0 in the menu, 1 playing, 2 the championship or
  // adventure is won (the rebase then goes to FS_xxWIN).
  int play_mode = 0;
  void RebaseToPlay();
  // Switches the flow and shows its first form (or `step`).
  void Go(const std::string& state, int step = 0);
  void AskQuit();
  // After the race (0x494218): reads `race` and switches to the result
  // flow of the game type.
  void AfterRace();
  // The race of the current round: [CHAMPIONSHIP] row type*5 + round, or
  // [ADVENTURE] row round (nullptr for Quick Race or past the end).
  const RaceEntry* CurrentRace() const;
  void Sound(const std::string& sfx_id);  // SNDFX_OK etc.
  void PlayMusic(const std::string& id);  // SNDCB_NORMAL etc.

  // The screen object for a form class (created once, kept).
  davi::Screen* Get(const std::string& cls);

 private:
  std::map<std::string, std::unique_ptr<davi::Screen>> screens_;
};

// Hooks the Europe Racer graphic names into the skin (skin.cpp).
void RegisterSkin(davi::Skin& skin, Texts& texts);

}  // namespace er
