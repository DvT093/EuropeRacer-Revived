// Screens after a race: who won (Quick Race), qualified / not qualified
// (Championship, Adventure). Each shows a text in TxtContent and a photo in
// WinnerImage, chosen from the race result (race.h).
//
// The winner's photo is ChaW<club> (the club's racer), Chaw8 the player and
// Chaw9 J.B. Black, the police chief. The racer's name is the club's
// LT_ED_<club> text (0x49ee2c).
#include <algorithm>
#include <cstdio>
#include <ctime>

#include "er/screens.h"
#include "er/table.h"

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

int CarClass(Game& g);

// The common part of the result forms (TBaseFrmWizard descendants).
class ResultScreen : public ErScreen {
 public:
  // `logo`: the form shows the player's club logo (CBLogo<club>, SetLogo
  // 0x492178): the Championship and Adventure forms do.
  ResultScreen(Game& g, const std::string& cls, bool logo) : ErScreen(g, cls), logo_(logo) {}
  void OnShow() override {
    ErScreen::OnShow();
    form.SetEnabled("BtnPrevious", false);
    CheckRecords();
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
    if (Is(w, "BtnNext")) {  // 0x4be06c: the next step of the result flow
      game.Sound("SNDFX_OK");
      game.NextPlain();
      return;
    }
    ErScreen::OnClick(w);
  }

 protected:
  // After-show hook (0x4c1bcc, 0x4be25c ...): a finished race may set the
  // lap record (best lap) and the track record (race time) of its track.
  // The original's popups for these are empty procedures: nothing shown.
  void CheckRecords() {
    if (!game.race.finished) return;
    int track = game.choices.game_type == "QR" ? game.choices.track
                                               : (game.CurrentRace() ? game.CurrentRace()->track : -1);
    const Track* t = game.data.TrackById(track);
    if (!t) return;
    const char* regions[] = {"TC_WESTEUROPE", "TC_MIDDLEEUROPE", "TC_SOUTHEUROPE"};
    int reg = 0;
    while (reg < 3 && t->region != regions[reg]) reg++;
    RecordSlot s;
    s.track = track;
    s.name = game.choices.player.substr(0, 8);
    s.car_type = CarClass(game);
    char b[16];
    std::time_t now = std::time(nullptr);
    std::strftime(b, sizeof b, "%Y-%m-%d", std::localtime(&now));
    s.date = b;
    s.time = uint32_t(game.race.best_lap);
    game.scores.Record(0, reg, s);
    s.time = uint32_t(game.race.tracktime);
    game.scores.Record(1, reg, s);
  }
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

// The player's car class: CH the chosen class, AD the car's (0x4a3828).
int CarClass(Game& g) {
  if (g.choices.game_type == "CH") return g.choices.ch_type;
  const Car* c = g.data.CarById(g.choices.car);
  const char* cls[] = {"CT_STANDARD", "CT_ATV", "CT_SPORT"};
  for (int i = 0; i < 3; i++)
    if (c && c->car_type == cls[i]) return i;
  return 0;
}

std::string RacerName(Game& g, int club) {
  return club >= 0 && size_t(club) < g.data.clubs.size() ? g.texts(g.data.clubs[club].opponent) : "";
}

// TFrmCH/ADResults (0x4c5ad4): the race's result in PnlRanking as layout B
// (TCtrlGameResults 0x4c5520: the player and the racers by position, name
// and time mm:ss:hh), and the winner's photo: Res1 when the player won,
// else cbimgdo<club>.
class RaceResults : public ErScreen {
 public:
  RaceResults(Game& g, const std::string& cls) : ErScreen(g, cls) {
    table_.Load(game.gamemenu, game.texts, "SCORE_LAYOUT-B", {"NAME", "TRACKTIME"});
  }
  void OnShow() override {
    ErScreen::OnShow();
    if (!app.skin.assets) return;
    form.SetPicture("LogoImage", app.skin.assets->Bitmap(Fmt("CBLogo%d", game.choices.club)));
    std::string pic = "Res1";
    if (game.race.position != 0)
      for (auto& r : game.race.racers)
        if (r.position == 0) pic = Fmt("cbimgdo%d", r.club);
    form.SetPicture("WinnerImage", app.skin.assets->Bitmap(pic));
  }
  void OnClick(davi::Widget& w) override {
    if (Is(w, "BtnNext")) {
      game.Sound("SNDFX_OK");
      game.NextPlain();
      return;
    }
    ErScreen::OnClick(w);
  }
  void PaintOver(SDL_Surface* canvas, int ox, int oy) override {
    davi::Widget* p = form.Get("PnlRanking");
    if (!p) return;
    struct Row {
      int position;
      std::string name;
      int time;
    };
    std::vector<Row> list{{game.race.position, game.choices.player, game.race.tracktime}};
    for (auto& r : game.race.racers) list.push_back({r.position, RacerName(game, r.club), r.time});
    std::stable_sort(list.begin(), list.end(), [](const Row& a, const Row& b) { return a.position < b.position; });
    std::vector<std::vector<std::string>> rows;
    for (auto& r : list) rows.push_back({r.name, Msh(r.time)});
    table_.Draw(app.skin, canvas, ox + p->r.x, oy + p->r.y, rows);
  }

 private:
  ScoreTable table_;
};

// TFrmCH/ADResultRanking (0x4bed10): adds the race to the standings
// (0x4be724), ranks them (0x4beb50) and shows them in PnlRanking as layout
// C (TCtrlEndRanking 0x4b6740: name, club, points, total time hh:mm:ss, in
// rank order). Photo: Res1 when the player leads, else cbimgdo<leader>.
// Next (0x4beff8) moves the league to its next race.
class Standings : public ErScreen {
 public:
  Standings(Game& g, const std::string& cls) : ErScreen(g, cls) {
    table_.Load(game.gamemenu, game.texts, "SCORE_LAYOUT-C", {"NAME", "CLUB", "SCORE", "TIME"});
  }
  void OnShow() override {
    ErScreen::OnShow();
    League* l = game.CurrentLeague();
    if (!l) return;
    // The original does this when the form is created, once per race.
    if (!game.race_counted) {
      l->AddResult(game.race, CarClass(game));
      l->Rank();
      l->UpgradeOpponents(CarClass(game), [](void* ctx, int club) -> const std::vector<int>* {
        Game& g = *static_cast<Game*>(ctx);
        if (size_t(club) >= g.data.clubs.size()) return nullptr;
        const auto& m = g.choices.game_type == "AD" ? g.data.ad_upgrades : g.data.ch_upgrades;
        auto it = m.find(g.data.clubs[club].type);
        return it == m.end() ? nullptr : &it->second;
      }, &game);
      game.race_counted = true;
    }
    if (!app.skin.assets) return;
    form.SetPicture("LogoImage", app.skin.assets->Bitmap(Fmt("CBLogo%d", game.choices.club)));
    int lead = l->Leader();
    form.SetPicture("WinnerImage", app.skin.assets->Bitmap(lead == l->player_club ? "Res1" : Fmt("cbimgdo%d", lead)));
  }
  void OnClick(davi::Widget& w) override {
    if (Is(w, "BtnNext")) {
      // 0x4beff8: the last race done -> won (+0x37a0 = 2), flow to FS_xxWIN.
      if (League* l = game.CurrentLeague(); l && !l->NextRound(game.LeagueRaces())) {
        game.play_mode = 2;
        game.RebaseToPlay();
      }
      game.Sound("SNDFX_OK");
      game.NextPlain();
      return;
    }
    ErScreen::OnClick(w);
  }
  void PaintOver(SDL_Surface* canvas, int ox, int oy) override {
    davi::Widget* p = form.Get("PnlRanking");
    League* l = game.CurrentLeague();
    if (!p || !l) return;
    std::vector<std::vector<std::string>> rows(5);
    auto put = [&](int club, const std::string& name) {
      const ClubStats& s = l->clubs[club];
      std::string cname = size_t(club) < game.data.clubs.size() ? game.texts(game.data.clubs[club].name) : "";
      if (s.rank >= 0 && s.rank < 5) rows[s.rank] = {name, cname, std::to_string(s.score), Hms(s.time)};
    };
    put(l->player_club, game.choices.player);
    for (int c : l->opponents) put(c, RacerName(game, c));
    table_.Draw(app.skin, canvas, ox + p->r.x, oy + p->r.y, rows);
  }

 private:
  ScoreTable table_;
};

// TFrmCH/ADShowStatus (0x4c0954, 0x4bb5xx): how many races are left
// (LT_MSG_ADSTATUS with the club's LT_TXT_CB_<club> and the number, or
// LT_MSG_ADSTATUS2 for the last one), the club's car for this class in
// ImgClub (car<id>a), and the standings in PnlRanking as layout D
// (TCtrlStatusRanking 0x4baef4: name, club, points, highest first).
// BtnRestart (0x4c0c38): "really restart?", then the league starts over
// from the movie of FS_CHNEW / FS_ADNEW.
class ShowStatus : public ErScreen {
 public:
  ShowStatus(Game& g, const std::string& cls) : ErScreen(g, cls) {
    table_.Load(game.gamemenu, game.texts, "SCORE_LAYOUT-D", {"NAME", "CLUB", "SCORE"});
  }
  void OnShow() override {
    ErScreen::OnShow();
    League* l = game.CurrentLeague();
    if (!l) return;
    static const char* kClubTexts[] = {"LT_TXT_CB_USA",     "LT_TXT_CB_JAPAN",  "LT_TXT_CB_RUSSIA", "LT_TXT_CB_GERMANY",
                                       "LT_TXT_CB_ENGLAND", "LT_TXT_CB_FRANCE", "LT_TXT_CB_ITALY",  "LT_TXT_CB_HOLLAND"};
    std::string club = game.choices.club >= 0 && game.choices.club < 8 ? game.texts(kClubTexts[game.choices.club]) : "";
    int left = game.LeagueRaces() - l->round;
    std::string msg = game.texts(left < 2 ? "LT_MSG_ADSTATUS2" : "LT_MSG_ADSTATUS");
    msg = FillName(msg, club);
    if (left >= 2) msg = FillName(msg, std::to_string(left));
    form.SetCaption("TxtContent", msg);
    if (app.skin.assets) {
      int car = ClubCarFor(CarClass(game));
      form.SetPicture("ImgClub", car >= 0 ? app.skin.assets->Bitmap(Fmt("car%.2da", car)) : nullptr);
    }
  }
  void OnClick(davi::Widget& w) override {
    if (Is(w, "BtnRestart")) {
      game.Ask(game.texts("LT_MSG_RESTART"), [this] {
        bool ad = game.choices.game_type == "AD";
        if (League* l = game.CurrentLeague()) l->Reset();  // 0x4a4f38
        game.flow.Set(ad ? "FS_ADNEW" : "FS_CHNEW");
        game.flow.SkipTo("TFrmPlayMovie");
        game.ShowCurrent();
      });
      return;
    }
    ErScreen::OnClick(w);
  }
  void PaintOver(SDL_Surface* canvas, int ox, int oy) override {
    davi::Widget* p = form.Get("PnlRanking");
    League* l = game.CurrentLeague();
    if (!p || !l) return;
    struct Row {
      std::string name, club;
      int score;
    };
    auto club_name = [&](int c) {
      return size_t(c) < game.data.clubs.size() ? game.texts(game.data.clubs[c].name) : std::string();
    };
    std::vector<Row> list{{game.choices.player, club_name(l->player_club), l->clubs[l->player_club].score}};
    for (int c : l->opponents) list.push_back({RacerName(game, c), club_name(c), l->clubs[c].score});
    std::stable_sort(list.begin(), list.end(), [](const Row& a, const Row& b) { return a.score > b.score; });
    std::vector<std::vector<std::string>> rows;
    for (auto& r : list) rows.push_back({r.name, r.club, std::to_string(r.score)});
    table_.Draw(app.skin, canvas, ox + p->r.x, oy + p->r.y, rows);
  }

 private:
  // The club's car of a class ([CLUBS], 0x49edcc +0xc + class*4).
  int ClubCarFor(int cls) const {
    if (size_t(game.choices.club) >= game.data.clubs.size()) return -1;
    const Club& c = game.data.clubs[game.choices.club];
    return cls == 1 ? c.car_atv : cls == 2 ? c.car_sport : c.car_standard;
  }
  ScoreTable table_;
};

// TFrmCH/ADResultWin (0x4bf408): a black form (zwart0) that enters the
// result in the high score table (0x4bf288: [SCORE_CC_<class>] /
// [SCORE_ADVENTURE] with a popup when it gets in, 0x490c44; not built yet),
// starts the league over (0x4a4f38) and goes on to the main menu.
class ResultWin : public ErScreen {
 public:
  ResultWin(Game& g, const std::string& cls) : ErScreen(g, cls) {}
  void OnShow() override {
    ErScreen::OnShow();
    app.After(0, [this] {
      League* l = game.CurrentLeague();
      auto finish = [this, l] {
        if (l) l->Reset();  // 0x4a4f38
        game.play_mode = 0;
        game.NextPlain();
      };
      int in = 0;
      if (l) {
        // 0x4bf288 / 0x4b7a94: the player's final standing into the table
        // of the class (CH) or the adventure table.
        const ClubStats& st = l->clubs[l->player_club];
        ScoreRow r;
        r.name = game.choices.player.substr(0, 8);
        r.score = st.score, r.position = st.rank, r.club = l->player_club, r.time = uint32_t(st.time);
        char b[16];
        std::time_t now = std::time(nullptr);
        std::strftime(b, sizeof b, "%Y-%m-%d", std::localtime(&now));
        r.date = b;
        int table = game.choices.game_type == "AD" ? 0 : 1 + std::clamp(game.choices.ch_type, 0, 2);
        in = game.scores.Insert(table, r);
      }
      if (in) {  // 0x490c44: TFrmRequester with LT_MSG_GAMEHIGHSCORE, OK only
        game.Message(game.texts("LT_MSG_GAMEHIGHSCORE"), finish);
      } else {
        finish();
      }
    }, this);
  }
};

}  // namespace

std::unique_ptr<davi::Screen> MakeResultScreen(Game& g, const std::string& key) {
  if (key == "tfrmqrresultwin") return std::make_unique<QRResultWin>(g);
  if (key == "tfrmchresultqualified") return std::make_unique<Qualification>(g, "TFrmCHResultQualified", "CH", true);
  if (key == "tfrmchresultdisqualified") return std::make_unique<Qualification>(g, "TFrmCHResultDisqualified", "CH", false);
  if (key == "tfrmadresultqualified") return std::make_unique<Qualification>(g, "TFrmADResultQualified", "AD", true);
  if (key == "tfrmadresultdisqualified") return std::make_unique<Qualification>(g, "TFrmADResultDisqualified", "AD", false);
  if (key == "tfrmchresults") return std::make_unique<RaceResults>(g, "TFrmCHResults");
  if (key == "tfrmadresults") return std::make_unique<RaceResults>(g, "TFrmADResults");
  if (key == "tfrmchresultranking") return std::make_unique<Standings>(g, "TFrmCHResultRanking");
  if (key == "tfrmadresultranking") return std::make_unique<Standings>(g, "TFrmADResultRanking");
  if (key == "tfrmchresultwin") return std::make_unique<ResultWin>(g, "TFrmCHResultWin");
  if (key == "tfrmadresultwin") return std::make_unique<ResultWin>(g, "TFrmADResultWin");
  if (key == "tfrmchshowstatus") return std::make_unique<ShowStatus>(g, "TFrmCHShowStatus");
  if (key == "tfrmadshowstatus") return std::make_unique<ShowStatus>(g, "TFrmADShowStatus");
  return nullptr;
}

}  // namespace er
