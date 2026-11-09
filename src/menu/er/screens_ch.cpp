// Championship and Adventure screens, plus the movie screen both flows use.
#include <cstdio>
#include <cstdlib>

#include "engine/dfm.h"
#include "engine/ini.h"
#include "engine/video.h"
#include "er/screens.h"

namespace er {

namespace {

bool Is(const davi::Widget& w, const char* name) { return davi::Lower(w.name) == davi::Lower(name); }

std::string Fmt(const char* fmt, int v) {
  char b[64];
  std::snprintf(b, sizeof b, fmt, v);
  return b;
}

const char* kClasses[] = {"CT_STANDARD", "CT_ATV", "CT_SPORT"};

// The race of the current round: [CHAMPIONSHIP] row type*5 + round, or
// [ADVENTURE] row round.
const RaceEntry* CurrentRace(Game& g) {
  const auto& list = g.choices.game_type == "AD" ? g.data.adventure : g.data.championship;
  size_t row = g.choices.game_type == "AD" ? g.choices.round : g.choices.ch_type * 5 + g.choices.round;
  return row < list.size() ? &list[row] : nullptr;
}

// TFrmPlayMovie: a full-screen movie from Langdata/<lang>/Media, then the
// next step of the flow (click or a key skips it). Which movie belongs to
// which flow state is not decompiled yet (GameMenu.ini [MOVIES] is empty on
// the CD); the mapping below follows the file names.
class PlayMovie : public ErScreen {
 public:
  PlayMovie(Game& g) : ErScreen(g, "TFrmPlayMovie") {
    form.w = 800, form.h = 600;
    for (auto& w : form.widgets) w->visible = false;
  }
  void OnShow() override {
    std::string st = davi::Lower(game.flow.State()), file;
    if (st == "fs_firsttime") file = "Intro.avi";
    else if (st == "fs_adnew") file = "Adv.avi";
    else if (st == "fs_adwin" || st == "fs_adending") file = "AdvWin.avi";
    else if (st == "fs_chnew") file = game.choices.ch_type == 1 ? "ATV.avi" : game.choices.ch_type == 2 ? "sport.avi" : "standard.avi";
    std::string path = file.empty() ? "" : davi::FindNoCase(game.media_dir, file);
    if (path.empty() || !movie_.Open(path, &app.audio, 1.0f)) {
      app.After(0, [this] { Done(); }, this);
      return;
    }
    app.audio.Pause(false);
  }
  void OnHide() override {
    movie_.Close();
    app.CancelTimers(this);
  }
  void Update(double) override {
    if (movie_.IsOpen() && !movie_.Update()) Done();
  }
  void PaintOver(SDL_Surface* canvas, int ox, int oy) override {
    SDL_FillSurfaceRect(canvas, nullptr, 0);
    if (SDL_Surface* f = movie_.Frame()) {
      SDL_Rect r{ox, oy, 800, 600};
      SDL_BlitSurfaceScaled(f, nullptr, canvas, &r, SDL_SCALEMODE_LINEAR);
    }
  }
  bool OnKey(SDL_Keycode) override {
    Done();
    return true;
  }
  void OnClick(davi::Widget&) override { Done(); }

 private:
  void Done() {
    if (done_) return;
    done_ = true;
    movie_.Close();
    game.Next();
    done_ = false;
  }
  davi::Video movie_;
  bool done_ = false;
};

// TFrmCHSelectType 0x4c04d4: standard, ATV or sport championship, info texts 0xe3-0xe5.
class CHSelectType : public ErScreen {
 public:
  CHSelectType(Game& g) : ErScreen(g, "TFrmCHSelectType") {
    BindRadios({"LT_INF_CC_STANDARD", "LT_INF_CC_ATV", "LT_INF_CC_SPORT"}, {});
    SetRadio(game.choices.ch_type);
  }
  void OnClick(davi::Widget& w) override {
    ErScreen::OnClick(w);
    game.choices.ch_type = CheckedRadio();
  }
};

// TFrmCHSelectDifficulty 0x4bff7c / TFrmADSelectDifficulty 0x4ba1a4: as Quick Race.
class SelectDifficulty : public ErScreen {
 public:
  SelectDifficulty(Game& g, const std::string& cls, const char* msg) : ErScreen(g, cls) {
    form.SetCaption("TxtContent", game.texts(msg));
    BindRadios({"LT_INF_DF_EASY", "LT_INF_DF_NORMAL", "LT_INF_DF_HARD"}, {"dfimg0", "dfimg1", "dfimg2"},
               "ImgDifficulty");
    SetRadio(game.choices.difficulty);
  }
  void OnClick(davi::Widget& w) override {
    ErScreen::OnClick(w);
    game.choices.difficulty = CheckedRadio();
  }
};

// TFrmCHSelectClub 0x4bf7e8 / TFrmADSelectClub 0x4b9a10: eight clubs; the
// club's driver (cbimgdO<n>), its photo (cbimgcO<n>) and info text
// (0xe9-0xf0, in [CLUBS] order).
class SelectClub : public ErScreen {
 public:
  SelectClub(Game& g, const std::string& cls) : ErScreen(g, cls) {
    form.SetCaption("TxtContent", game.texts("LT_MSG_CLUB"));  // text 0x10
    std::vector<std::string> texts, drivers, photos;
    for (size_t i = 0; i < game.data.clubs.size(); i++) texts.push_back(game.data.clubs[i].info);
    BindRadios(texts, {});
    SetRadio(game.choices.club);
    Pictures();
  }
  void OnClick(davi::Widget& w) override {
    ErScreen::OnClick(w);
    game.choices.club = CheckedRadio();
    Pictures();
  }

 private:
  void Pictures() {
    if (!app.skin.assets) return;
    form.SetPicture("ImgDriver", app.skin.assets->Bitmap(Fmt("cbimgdO%d", game.choices.club)));
    form.SetPicture("ImgClub", app.skin.assets->Bitmap(Fmt("cbimgcO%d", game.choices.club)));
  }
};

// TFrmCHShowTrack 0x4bd080 / TFrmADShowTrack 0x4b48cc: the track's
// landmark collage (LMark<track>) and its story; the Adventure adds the car
// advice (texts 0x150-0x152).
class ShowTrack : public ErScreen {
 public:
  ShowTrack(Game& g, const std::string& cls) : ErScreen(g, cls) {}
  void OnShow() override {
    ErScreen::OnShow();
    const RaceEntry* r = CurrentRace(game);
    const Track* t = r ? game.data.TrackById(r->track) : nullptr;
    if (!t) return;
    if (app.skin.assets) form.SetPicture("LandMarkImg", app.skin.assets->Bitmap(Fmt("LMark%d", t->id)));
    std::string text = game.texts(Fmt("LT_INF_TRACK%.2d", t->id));
    if (game.choices.game_type == "AD") {
      const char* adv = t->car_type == "CT_ATV" ? "LT_TXT_CC_ATV" : t->car_type == "CT_SPORT" ? "LT_TXT_CC_SPORT" : "LT_TXT_CC_STANDARD";
      text += "\n\n" + game.texts(adv);
    }
    form.SetCaption("TxtContent", text);
  }
};

// The car properties as bars (TGDIMultiProgressBar), see QRSelectCar.
void PaintBars(ErScreen& s, Game& game, const Car* c, int stage, SDL_Surface* canvas, int ox, int oy) {
  davi::Widget* mpb = s.form.Get("MpbCarProperties");
  if (!mpb || !c || !s.app.skin.assets) return;
  SDL_Surface* on = s.app.skin.assets->Bitmap("Stats1");
  SDL_Surface* off = s.app.skin.assets->Bitmap("Stats0");
  const char* labels[] = {"LT_CPROP_SPEED", "LT_CPROP_ACCELERATION", "LT_CPROP_GRIP", "LT_CPROP_STRENGTH"};
  davi::Font* f = s.app.skin.GetFont(4);
  for (int p = 0; p < 4; p++) {
    int y = oy + mpb->r.y + 5 + p * 19;
    int v = size_t(stage * 4 + p) < c->props.size() ? c->props[stage * 4 + p] / 10 : 0;
    if (f) f->DrawLine(canvas, ox + mpb->r.x + 5, y, game.texts(labels[p]), 11, {0, 0, 0, 255});
    for (int i = 0; i < 10; i++)
      if (SDL_Surface* b = i < v ? on : off) {
        SDL_Rect d{ox + mpb->r.x + 95 + i * b->w, y, b->w, b->h};
        SDL_BlitSurface(b, nullptr, canvas, &d);
      }
  }
}

// The club's car of a class: [CLUBS] standard/sport/ATV car ids.
int ClubCar(Game& g, int cls) {
  if (size_t(g.choices.club) >= g.data.clubs.size()) return -1;
  const Club& c = g.data.clubs[g.choices.club];
  return cls == 1 ? c.car_atv : cls == 2 ? c.car_sport : c.car_standard;
}

// TFrmADSelectCar 0x4b94a0: the club's three cars as a carousel, with the
// MensMal note and the property bars.
class ADSelectCar : public ErScreen {
 public:
  ADSelectCar(Game& g) : ErScreen(g, "TFrmADSelectCar"), car_(*this) {
    form.SetCaption("TxtContent", game.texts("LT_MSG_QRCAR"));  // text 0xd
    car_.prev = "ImgCarPrev", car_.cur = "ImgCarCur", car_.next = "ImgCarNext";
    car_.info = "TxtInfo";
    car_.btn_prev = "BtnCarPrev", car_.btn_next = "BtnCarNext";
    // BgCar already has the blueprint sheet the bars sit on.
    form.SetVisible("ImgProperties", false);
  }
  void OnShow() override {
    ErScreen::OnShow();
    car_.items.clear();
    for (int k = 0; k < 3; k++)
      if (const Car* c = game.data.CarById(ClubCar(game, k)))
        car_.items.push_back({c->id, Fmt("car%.2da", c->id), "", game.texts(c->info)});
    car_.Select(car_.IndexOf(game.choices.car));
    game.choices.car = car_.Current();
  }
  void OnClick(davi::Widget& w) override {
    if (car_.Click(w)) {
      game.choices.car = car_.Current();
      return;
    }
    ErScreen::OnClick(w);
  }
  void PaintOver(SDL_Surface* canvas, int ox, int oy) override {
    PaintBars(*this, game, game.data.CarById(car_.Current()), 0, canvas, ox, oy);
  }

 private:
  Carousel car_;
};

// TFrmCHGarage 0x4c10a8 / TFrmADGarage 0x4bcb3c: the club photo
// (cbimgcO<club>), the car (car<id>a) with its property bars, and the
// garage offers as a carousel: repair 5% / 10% (GRep10/50), nitro 1 / 5
// litre (GNit10/50), the next upgrade pack (GPak1-3). Prices and buying are
// not decompiled yet: the texts show "?" for the amounts and Kopen is off.
class Garage : public ErScreen {
 public:
  Garage(Game& g, const std::string& cls) : ErScreen(g, cls), car_(*this) {
    car_.prev = "ImgCarPrev", car_.cur = "ImgCarCur", car_.next = "ImgCarNext";
    car_.name = "lblUpgradeCost", car_.info = "TxtInfo";
    car_.btn_prev = "BtnCarPrev", car_.btn_next = "BtnCarNext";
    struct Offer { const char* img; const char* text; };
    const Offer offers[] = {{"GRep10", "LT_GRG_REPAIRSMALL"}, {"GRep50", "LT_GRG_REPAIRLARGE"},
                            {"GNit10", "LT_GRG_NITROSMALL"}, {"GNit50", "LT_GRG_NITROLARGE"},
                            {"GPak1", "LT_GRG_UPGRADE1"}};
    int id = 0;
    for (auto& o : offers) {
      std::string t = game.texts(o.text);
      for (size_t p; (p = t.find("%s")) != std::string::npos;) t.replace(p, 2, "?");
      for (size_t p; (p = t.find("%%")) != std::string::npos;) t.replace(p, 2, "%");
      car_.items.push_back({id++, o.img, "", t});
    }
    form.SetEnabled("BtnBuy", false);
  }
  void OnShow() override {
    ErScreen::OnShow();
    if (app.skin.assets) {
      // The background has a 160x160 hole at ImgClub: the club photo goes
      // there (CBLogo<n> is only 133x70 and leaves the hole black).
      form.SetPicture("ImgClub", app.skin.assets->Bitmap(Fmt("cbimgcO%d", game.choices.club)));
      int car = game.choices.game_type == "AD" ? game.choices.car : ClubCar(game, game.choices.ch_type);
      form.SetPicture("ImgProperties", app.skin.assets->Bitmap(Fmt("car%.2da", car)));
    }
    car_.Select(car_.index);
  }
  void OnClick(davi::Widget& w) override {
    if (car_.Click(w)) return;
    ErScreen::OnClick(w);
  }
  void PaintOver(SDL_Surface* canvas, int ox, int oy) override {
    int car = game.choices.game_type == "AD" ? game.choices.car : ClubCar(game, game.choices.ch_type);
    PaintBars(*this, game, game.data.CarById(car), 0, canvas, ox, oy);
  }

 private:
  Carousel car_;
};

}  // namespace

std::unique_ptr<davi::Screen> MakeChampionshipScreen(Game& g, const std::string& key) {
  if (key == "tfrmplaymovie") return std::make_unique<PlayMovie>(g);
  if (key == "tfrmchselecttype") return std::make_unique<CHSelectType>(g);
  if (key == "tfrmchselectdifficulty") return std::make_unique<SelectDifficulty>(g, "TFrmCHSelectDifficulty", "LT_MSG_CHDIFFICULTY");
  if (key == "tfrmadselectdifficulty") return std::make_unique<SelectDifficulty>(g, "TFrmADSelectDifficulty", "LT_MSG_QRDIFFICULTY");
  if (key == "tfrmchselectclub") return std::make_unique<SelectClub>(g, "TFrmCHSelectClub");
  if (key == "tfrmadselectclub") return std::make_unique<SelectClub>(g, "TFrmADSelectClub");
  if (key == "tfrmchshowtrack") return std::make_unique<ShowTrack>(g, "TFrmCHShowTrack");
  if (key == "tfrmadshowtrack") return std::make_unique<ShowTrack>(g, "TFrmADShowTrack");
  if (key == "tfrmadselectcar") return std::make_unique<ADSelectCar>(g);
  if (key == "tfrmchgarage") return std::make_unique<Garage>(g, "TFrmCHGarage");
  if (key == "tfrmadgarage") return std::make_unique<Garage>(g, "TFrmADGarage");
  return nullptr;
}

}  // namespace er

namespace er {

namespace {

bool IsW(const davi::Widget& w, const char* name) { return davi::Lower(w.name) == davi::Lower(name); }

// A row of tab radios, each showing one panel.
struct Tabs {
  std::vector<std::pair<std::string, std::string>> tabs;  // radio -> panel
  void Show(ErScreen& s, const std::string& radio) {
    for (auto& [r, p] : tabs) {
      bool on = davi::Lower(r) == davi::Lower(radio);
      s.form.W(r).checked = on;
    }
    for (auto& [r, p] : tabs)
      if (davi::Lower(r) != davi::Lower(radio)) s.SetPanel(p, false);
    for (auto& [r, p] : tabs)
      if (davi::Lower(r) == davi::Lower(radio)) s.SetPanel(p, true);
  }
  bool Click(ErScreen& s, davi::Widget& w) {
    for (auto& [r, p] : tabs)
      if (IsW(w, r.c_str())) {
        Show(s, r);
        return true;
      }
    return false;
  }
};

// TFrmOptions 0x48c904: four tabs (controls, resolution, sound, game
// speed), one panel each. Pictures set in code: opct<n> per control type,
// OpRe<n> per resolution, "sld" as the slider track (needle gdfOpNeedle).
// The values themselves are not read from or written to game.ini yet.
class Options : public ErScreen {
 public:
  Options(Game& g) : ErScreen(g, "TFrmOptions") {
    tabs_.tabs = {{"btnControls", "pnlControls"}, {"btnResolution", "pnlResolution"},
                  {"btnSound", "pnlSound"}, {"btnGameSpeed", "pnlSpeed"}};
    for (const char* s : {"sliMusicVolume", "sliSoundEffects", "sliVoices"})
      if (davi::Widget* w = form.Get(s); w && app.skin.assets) {
        w->picture = app.skin.assets->Bitmap("sld");
        w->pos = 75;
      }
    Check("btnKeyboard");
    Check("btnMedRes");
    Pictures();
  }
  void OnShow() override {
    ErScreen::OnShow();
    tabs_.Show(*this, "btnControls");
  }
  void OnClick(davi::Widget& w) override {
    if (tabs_.Click(*this, w)) {
      game.Sound("SNDFX_RADIO");
      return;
    }
    ErScreen::OnClick(w);
    Pictures();
  }

 private:
  void Check(const char* name) {
    if (davi::Widget* w = form.Get(name)) w->checked = true;
  }
  void Pictures() {
    if (!app.skin.assets) return;
    const char* ctl[] = {"btnKeyboard", "btnGamePad", "btnJoystick", "btnSteeringWheel"};
    for (int i = 0; i < 4; i++)
      if (davi::Widget* w = form.Get(ctl[i]); w && w->checked)
        form.SetPicture("imgControlType", app.skin.assets->Bitmap(Fmt("opct%d", i)));
    const char* res[] = {"btnLowRes", "btnMedRes", "btnHighRes"};
    for (int i = 0; i < 3; i++)
      if (davi::Widget* w = form.Get(res[i]); w && w->checked)
        form.SetPicture("imgResolution", app.skin.assets->Bitmap(Fmt("OpRe%d", i)));
  }
  Tabs tabs_;
};

// TFrmHighscore 0x488864: tabs for the Adventure and the three
// championships (page 1) and the lap and track records (page 5, with a
// region tab per league).
//
// Score data (MdlScores 0x495c40): [SCORE_ADVENTURE] and [SCORE_CC_*] hold
// five "name,score,position" rows (0x495928). The club is Random(7) and the
// total time a fixed 0x05265818 ms; both are set at load, not read.
// [SCORE_LAP]/[SCORE_TRACK] hold "track,name" rows (0x4959fc), grouped per
// region of the track (up to 10 per region), car type Random(3), time
// 3599990 ms (or the previous row's time).
//
// Tables (TCtrlScore, 0x4846a8, font Hilde): page 1 uses [SCORE_LAYOUT-A]
// (0x4850f4) with columns name, score, position + 1, club, time as
// hh:mm:ss (0x468018; fill 0x4856ac/0x485930). Page 5 uses [SCORE_LAYOUT-E]
// (0x486084) with name, track, car type, time as mm:ss:hh (0x4680c8; fill
// 0x4865ac lap, 0x486844 track), at most 7 rows.
class Highscore : public ErScreen {
  void Row(std::initializer_list<const char*> names) {
    davi::Widget* prev = nullptr;
    for (const char* n : names) {
      davi::Widget* w = form.Get(n);
      if (w && prev) w->r.x = prev->r.x + prev->r.w, w->r.y = prev->r.y;
      if (w) prev = w;
    }
  }
  struct Score {
    std::string name;
    int score = 0, position = 0, club = 0, time = 0;
  };
  struct Record {
    int track = 0, car_type = 0, time = 0;
    std::string name;
  };
  struct TableLayout {
    int y = 0, title_h = 0, row_h = 0, title_px = 0, row_px = 0;
    std::vector<std::pair<int, std::string>> cols;  // x, title text
  };

  static std::string Hms(int ms) {  // 0x468018
    char b[32];
    int s = ms / 1000;
    std::snprintf(b, sizeof b, "%.2d:%.2d:%.2d", s / 3600 % 60, s / 60 % 60, s % 60);
    return b;
  }
  static std::string Msh(int ms) {  // 0x4680c8
    char b[32];
    int s = ms / 1000;
    std::snprintf(b, sizeof b, "%.2d:%.2d:%.2d", s / 60 % 60, s % 60, ms % 1000 / 10);
    return b;
  }

  TableLayout ReadLayout(const std::string& sec, std::initializer_list<const char*> cols) {
    const davi::LayeredIni& ini = game.gamemenu;
    TableLayout l;
    l.y = ini.GetInt(sec, "CTT_Y");
    l.title_h = ini.GetInt(sec, "CTT_TITLEHEIGHT");
    l.row_h = ini.GetInt(sec, "CTT_CONTROLHEIGHT");
    l.title_px = -ini.GetInt(sec, "CTT_TITLEFONTHEIGHT");
    l.row_px = -ini.GetInt(sec, "CTT_CONTROLFONTHEIGHT");
    for (const char* c : cols)
      l.cols.push_back({ini.GetInt(sec, std::string("CTT_") + c + "_X"),
                        game.texts(davi::Trim(ini.Get(sec, std::string("CTT_") + c + "_TITLE")))});
    return l;
  }

  void LoadScores() {
    const davi::LayeredIni& ini = game.gamemenu;
    const char* secs[] = {"SCORE_ADVENTURE", "SCORE_CC_STANDARD", "SCORE_CC_ATV", "SCORE_CC_SPORT"};
    for (int t = 0; t < 4; t++)
      for (int i = 0; i < 5; i++) {
        std::vector<std::string> f = Fields(ini.Get(secs[t], std::to_string(i)));
        Score s;
        if (!f.empty()) s.name = f[0].substr(0, 7);  // string[7]
        if (f.size() > 1) s.score = std::atoi(f[1].c_str());
        if (f.size() > 2) s.position = std::atoi(f[2].c_str());
        s.club = std::rand() % 7;
        s.time = 0x05265818;
        scores_[t].push_back(s);
      }
    const char* recs[] = {"SCORE_LAP", "SCORE_TRACK"};
    const char* regions[] = {"TC_WESTEUROPE", "TC_MIDDLEEUROPE", "TC_SOUTHEUROPE"};
    for (int t = 0; t < 2; t++) {
      int n = ini.GetInt(recs[t], "Count");
      for (int i = 0; i < n; i++) {
        std::vector<std::string> f = Fields(ini.Get(recs[t], std::to_string(i)));
        if (f.size() < 2) continue;
        Record r;
        r.track = std::atoi(f[0].c_str());
        r.name = f[1].substr(0, 7);
        r.car_type = std::rand() % 3;
        const Track* tr = game.data.TrackById(r.track);
        int reg = 0;
        while (tr && reg < 3 && tr->region != regions[reg]) reg++;
        if (reg >= 3) continue;
        auto& list = records_[t][reg];
        if (list.size() >= 10) continue;
        r.time = list.empty() ? 3599990 : list.back().time;
        list.push_back(r);
      }
    }
  }

  void DrawTable(SDL_Surface* canvas, int x, int y, const TableLayout& l,
                 const std::vector<std::vector<std::string>>& rows) {
    davi::Font* f = app.skin.GetFont(1);  // Hilde
    if (!f) return;
    SDL_Color black{0, 0, 0, 255};
    for (auto& [cx, title] : l.cols) f->DrawLine(canvas, x + cx, y + l.y, title, l.title_px, black);
    for (size_t r = 0; r < rows.size(); r++)
      for (size_t c = 0; c < rows[r].size() && c < l.cols.size(); c++)
        f->DrawLine(canvas, x + l.cols[c].first, y + l.y + l.title_h + int(r) * l.row_h, rows[r][c], l.row_px,
                    black);
  }

 public:
  Highscore(Game& g) : ErScreen(g, "TFrmHighscore") {
    tabs_.tabs = {{"BtnTab1", "PnPage1"}, {"BtnTab2", "PnPage1"}, {"BtnTab3", "PnPage1"},
                  {"BtnTab4", "PnPage1"}, {"BtnTab5", "PnPage5"}, {"BtnTab6", "PnPage5"}};
    if (davi::Widget* r = form.Get("Pg5Region1")) r->checked = true;
    score_layout_ = ReadLayout("SCORE_LAYOUT-A", {"NAME", "SCORE", "POSITION", "CLUB", "TIME"});
    record_layout_ = ReadLayout("SCORE_LAYOUT-E", {"NAME", "TRACK", "CARTYPE", "TRACKTIME"});
    LoadScores();
  }
  // The tab control (0x4875f0) lines its tabs up from the first one.
  void Layout() override {
    ErScreen::Layout();
    Row({"BtnTab1", "BtnTab2", "BtnTab3", "BtnTab4", "BtnTab5", "BtnTab6"});
    Row({"Pg5Region1", "Pg5Region2", "Pg5Region3"});
  }
  void OnShow() override {
    ErScreen::OnShow();
    SetPanel("PnPage1", false);
    SetPanel("PnPage5", false);
    tabs_.Show(*this, "BtnTab1");
    tab_ = 0;
  }
  void OnClick(davi::Widget& w) override {
    const char* tabs[] = {"BtnTab1", "BtnTab2", "BtnTab3", "BtnTab4", "BtnTab5", "BtnTab6"};
    for (int i = 0; i < 6; i++)
      if (IsW(w, tabs[i])) tab_ = i;
    const char* regions[] = {"Pg5Region1", "Pg5Region2", "Pg5Region3"};
    for (int i = 0; i < 3; i++)
      if (IsW(w, regions[i])) region_ = i;
    if (tabs_.Click(*this, w)) {
      game.Sound("SNDFX_RADIO");
      return;
    }
    ErScreen::OnClick(w);
  }
  void PaintOver(SDL_Surface* canvas, int ox, int oy) override {
    if (tab_ < 4) {
      davi::Widget* p = form.Get("PnPage1");
      if (!p) return;
      std::vector<std::vector<std::string>> rows;
      for (const Score& s : scores_[tab_]) {
        std::string club = size_t(s.club) < game.data.clubs.size() ? game.texts(game.data.clubs[s.club].name) : "";
        rows.push_back({s.name, std::to_string(s.score), std::to_string(s.position + 1), club, Hms(s.time)});
      }
      DrawTable(canvas, ox + p->r.x, oy + p->r.y, score_layout_, rows);
    } else {
      davi::Widget *p = form.Get("PnPage5"), *pg = form.Get("Pg5Page1");
      if (!p || !pg) return;
      std::vector<std::vector<std::string>> rows;
      const char* types[] = {"LT_CD_STANDARD", "LT_CD_ATV", "LT_CD_SPORT"};
      for (const Record& r : records_[tab_ - 4][region_]) {
        if (rows.size() >= 7) break;
        const Track* t = game.data.TrackById(r.track);
        rows.push_back({r.name, t ? game.texts(t->name) : "", game.texts(types[r.car_type]), Msh(r.time)});
      }
      DrawTable(canvas, ox + pg->r.x, oy + pg->r.y, record_layout_, rows);
    }
  }

 private:
  Tabs tabs_;
  int tab_ = 0, region_ = 0;
  TableLayout score_layout_, record_layout_;
  std::vector<Score> scores_[4];
  std::vector<Record> records_[2][3];
};

}  // namespace

std::unique_ptr<davi::Screen> MakeDialogScreen(Game& g, const std::string& key) {
  if (key == "tfrmoptions") return std::make_unique<Options>(g);
  if (key == "tfrmhighscore") return std::make_unique<Highscore>(g);
  return nullptr;
}

}  // namespace er
