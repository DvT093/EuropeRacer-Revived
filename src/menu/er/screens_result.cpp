// Screens after a race: who won (Quick Race), qualified / not qualified
// (Championship, Adventure). Each shows a text in TxtContent and a photo in
// WinnerImage, chosen from the race result (race.h).
//
// The winner's photo is ChaW<club> (the club's racer), Chaw8 the player and
// Chaw9 J.B. Black, the police chief. The racer's name is the club's
// LT_ED_<club> text (0x49ee2c).
#include <cstdio>

#include "er/screens.h"

namespace er {

namespace {

bool Is(const davi::Widget& w, const char* name) { return davi::Lower(w.name) == davi::Lower(name); }

std::string Fmt(const char* fmt, int v) {
  char b[64];
  std::snprintf(b, sizeof b, fmt, v);
  return b;
}

std::string FillName(std::string msg, const std::string& who) {
  size_t p = msg.find("%s");
  if (p != std::string::npos) msg.replace(p, 2, who);
  return msg;
}

// The common part of the result forms (TBaseFrmWizard descendants).
class ResultScreen : public ErScreen {
 public:
  // `logo`: the form shows the player's club logo (CBLogo<club>, SetLogo
  // 0x492178): the Championship and Adventure forms do.
  ResultScreen(Game& g, const std::string& cls, bool logo) : ErScreen(g, cls), logo_(logo) {}
  void OnShow() override {
    ErScreen::OnShow();
    form.SetEnabled("BtnPrevious", false);
    if (logo_ && app.skin.assets) form.SetPicture("LogoImage", app.skin.assets->Bitmap(Fmt("CBLogo%d", game.choices.club)));
    Fill();
  }
  // btnTryAgain (0x4c168c, 0x4bd7ac): shows TFrmRace again, which runs the
  // same race; the flow state is left as it is (AfterRace sets the next).
  void OnClick(davi::Widget& w) override {
    if (Is(w, "btnTryAgain")) {
      app.Show(game.Get("TFrmRace"));
      return;
    }
    ErScreen::OnClick(w);
  }

 protected:
  virtual void Fill() = 0;
  // The racer that won (position 0) as {name, photo}; empty if none.
  std::pair<std::string, std::string> RacerWinner() const {
    for (auto& r : game.race.racers) {
      if (r.position != 0) continue;
      std::string who = r.club >= 0 && size_t(r.club) < game.data.clubs.size()
                            ? game.texts(game.data.clubs[r.club].opponent) : "";
      return {who, Fmt("ChaW%d", r.club)};
    }
    return {"", ""};
  }
  void Show(const std::string& text, const std::string& pic) {
    form.SetCaption("TxtContent", text);
    if (!app.skin.assets) return;
    SDL_Surface* s = pic.empty() ? nullptr : app.skin.assets->Bitmap(pic);
    if (!s) SDL_Log("menu: Could not find the qinner TGA! (%s)", pic.c_str());  // sic
    form.SetPicture("WinnerImage", s);
  }

 private:
  bool logo_;
};

// TFrmQRResultWin 0x4c1584 / 0x4c16ec. The player: LT_MSG_QR_YOUWON with
// Chaw8. A duel against the police: LT_MSG_QR_PS_WON with J.B. Black and
// Chaw9 (read as QR type 1 and opponent group 1: 0x4a4400/0x4a4128, not
// proven). Else the winning racer. "Try again" (0x4c168c) races the same
// track again; Next goes on to the track choice (FS_QRWIN step 1).
class QRResultWin : public ResultScreen {
 public:
  QRResultWin(Game& g) : ResultScreen(g, "TFrmQRResultWin", false) {}

 protected:
  void Fill() override {
    if (game.race.Winner() == 0) {
      Show(game.texts("LT_MSG_QR_YOUWON"), "Chaw8");  // text 0x31
    } else if (game.choices.qr_type == 1 && game.choices.opponent == 8) {
      Show(FillName(game.texts("LT_MSG_QR_PS_WON"), game.texts("LT_ED_CHIEFPOLICE")), "Chaw9");  // 0x32, 0x55
    } else {
      auto [who, pic] = RacerWinner();
      Show(FillName(game.texts("LT_MSG_QR_PS_WON"), who), pic);
    }
  }
};

// TFrmCH/ADResultQualified (0x4bdd24, AD alike): won -> LT_MSG_xx_YOUWON
// with Chaw8, else LT_MSG_xx_QUALIFIED with the winner.
// TFrmCH/ADResultDisqualified: always LT_MSG_xx_NOT_QUALIFIED with the
// winner.
class Qualification : public ResultScreen {
 public:
  Qualification(Game& g, const std::string& cls, std::string mode, bool qualified)
      : ResultScreen(g, cls, true), mode_(std::move(mode)), qualified_(qualified) {}

 protected:
  void Fill() override {
    if (qualified_ && game.race.position == 0) {
      Show(game.texts("LT_MSG_" + mode_ + "_YOUWON"), "ChaW8");
      return;
    }
    auto [who, pic] = RacerWinner();
    Show(FillName(game.texts("LT_MSG_" + mode_ + (qualified_ ? "_QUALIFIED" : "_NOT_QUALIFIED")), who), pic);
  }

 private:
  std::string mode_;
  bool qualified_;
};

}  // namespace

std::unique_ptr<davi::Screen> MakeResultScreen(Game& g, const std::string& key) {
  if (key == "tfrmqrresultwin") return std::make_unique<QRResultWin>(g);
  if (key == "tfrmchresultqualified") return std::make_unique<Qualification>(g, "TFrmCHResultQualified", "CH", true);
  if (key == "tfrmchresultdisqualified") return std::make_unique<Qualification>(g, "TFrmCHResultDisqualified", "CH", false);
  if (key == "tfrmadresultqualified") return std::make_unique<Qualification>(g, "TFrmADResultQualified", "AD", true);
  if (key == "tfrmadresultdisqualified") return std::make_unique<Qualification>(g, "TFrmADResultDisqualified", "AD", false);
  return nullptr;
}

}  // namespace er
