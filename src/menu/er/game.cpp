#include "er/game.h"

#include <algorithm>
#include <cstring>

#include "engine/dfm.h"
#include "er/screens.h"

namespace er {

namespace {

bool Is(const davi::Widget& w, const char* name) { return davi::Lower(w.name) == davi::Lower(name); }

// TFrmMainWindow: the three game types are radio buttons; Next follows the
// chosen one (BtnRadio1 Quick Race, 2 Championship, 3 Adventure).
// Positions as the original's layout methods compute them.
void Place(davi::Widget* w, int x, int y) {
  if (w) w->r.x = x, w->r.y = y;
}
bool Shown(davi::Widget* w) { return w && w->visible; }

class MainWindow : public ErScreen {
 public:
  // 0x4ab57c: wizard layout, then Options/Player list/Internet/High score
  // in a row from the left edge, 2 px apart, 4 px above the bottom.
  void Layout() override {
    ErScreen::Layout();
    int x = 0;
    for (const char* n : {"BtnOption", "BtnPlayerList", "BtnInternet", "BtnHighScore"})
      if (davi::Widget* b = form.Get(n)) {
        Place(b, x, form.h - b->r.h - 4);
        x += b->r.w + 2;
      }
  }
  MainWindow(Game& g) : ErScreen(g, "TFrmMainWindow") {
    // 0x4ab8bc: an info text per game type (texts 0xd8-0xda).
    BindRadios({"LT_INF_GT_QUICKRACE", "LT_INF_GT_CHAMPIONSHIP", "LT_INF_GT_ADVENTURE"}, {});
    form.SetVisible("BtnPrevious", false);
  }
  void OnClick(davi::Widget& w) override {
    if (Is(w, "BtnNext")) {
      game.Sound("SNDFX_OK");
      std::string type = form.W("BtnRadio2").checked ? "CH" : form.W("BtnRadio3").checked ? "AD" : "QR";
      game.choices.game_type = type;
      game.play_mode = 1;
      // A new championship/adventure: every standing back to the start
      // (0x4a2be8). There are no saved games yet to continue.
      if (type != "QR") {
        for (auto& l : game.ch) l.Reset();
        game.ad.Reset();
      }
      // A new championship/adventure until saved games exist (FS_CHSTANDARD /
      // FS_ADSTANDARD continue one).
      game.Go(type == "QR" ? "FS_QRSTANDARD" : type == "CH" ? "FS_CHNEW" : "FS_ADNEW", 1);
      return;
    }
    // BtnOptionClick 0x4ab9e8, BtnPlayerListClick 0x4aba2c, BtnInternetClick
    // 0x4abacc, BtnHighScoreClick 0x4abb10: modal dialogs.
    const char* dialogs[][2] = {{"BtnOption", "TFrmOptions"}, {"BtnPlayerList", "TFrmPlayerList"},
                                {"BtnInternet", "TFrmInternet"}, {"BtnHighScore", "TFrmHighscore"}};
    for (auto& d : dialogs)
      if (Is(w, d[0])) {
        game.Sound("SNDFX_OK");
        davi::Screen* s = game.Get(d[1]);
        if (s->built) app.ShowModal(s);
        return;
      }
    ErScreen::OnClick(w);
  }
};

}  // namespace

ErScreen::ErScreen(Game& g, const std::string& cls) : davi::Screen(g.app, cls), game(g) {
  if (davi::Widget* p = form.Get("BtnPrevious")) previous_allowed_ = p->visible;
  // SetBackground (0x492080) sizes the form to its background picture.
  if (davi::Widget* bg = form.Get("BgndImage"); bg && bg->picture) {
    form.w = bg->picture->w;
    form.h = bg->picture->h;
  }
  // Plain VCL controls (TLabel, TPanel, TMemo, ...) on these DirectDraw
  // forms are design-time helpers; the GDI layer draws over them.
  for (auto& w : form.widgets)
    if (w->cls.rfind("TGDI", 0) != 0) w->visible = false;
  // BlackImage: a fade layer, hidden until used.
  for (auto& w : form.widgets)
    if (davi::Lower(w->name) == "blackimage") w->visible = false;  // inherited + redeclared: two of them
  // No help file in this port (ER.hlp is a WinHelp file).
  if (davi::Widget* h = form.Get("BtnHelp")) h->visible = false;
}

void ErScreen::BindRadios(std::vector<std::string> texts, std::vector<std::string> pictures, const std::string& image) {
  radio_texts_ = std::move(texts);
  radio_pics_ = std::move(pictures);
  radio_image_ = image;
  if (CheckedRadio() == 0 && form.Get("BtnRadio1")) form.W("BtnRadio1").checked = true;
  ApplyRadio();
}

void ErScreen::SetRadio(int i) {
  for (int k = 0; k < 8; k++)
    if (davi::Widget* r = form.Get("BtnRadio" + std::to_string(k + 1))) r->checked = k == i;
  ApplyRadio();
}

int ErScreen::CheckedRadio() const {
  for (int k = 0; k < 8; k++)
    if (davi::Widget* r = const_cast<davi::Form&>(form).Get("BtnRadio" + std::to_string(k + 1)); r && r->checked)
      return k;
  return 0;
}

void ErScreen::ApplyRadio() {
  int i = CheckedRadio();
  if (i < int(radio_texts_.size())) form.SetCaption("TxtInfo", game.texts(radio_texts_[i]));
  if (!radio_image_.empty() && i < int(radio_pics_.size()) && app.skin.assets)
    form.SetPicture(radio_image_, app.skin.assets->Bitmap(radio_pics_[i]));
}

void ErScreen::SetPanelVisible(const std::string& panel, bool v) {
  std::string p = davi::Lower(panel);
  auto inside = [&](davi::Widget* w) {
    for (int depth = 0; w && depth < 16; depth++) {
      if (davi::Lower(w->name) == p) return true;
      w = w->parent.empty() ? nullptr : form.Get(w->parent);
    }
    return false;
  };
  for (auto& w : form.widgets)
    if (inside(w.get())) w->visible = v;
}

std::string ErScreen::Format(const std::string& msg) const {
  std::string out = msg;
  size_t p = out.find("%s");
  if (p != std::string::npos) out.replace(p, 2, game.choices.player);
  return out;
}

void Carousel::Select(int i) {
  // The current polaroid lies on top of its neighbours (some forms list it
  // before them in the DFM).
  auto& ws = screen.form.widgets;
  for (size_t k = 0; !cur.empty() && k + 1 < ws.size(); k++)
    if (davi::Lower(ws[k]->name) == davi::Lower(cur)) {
      for (const std::string* nb : {&prev, &next})
        for (size_t m = k + 1; m < ws.size(); m++)
          if (!nb->empty() && davi::Lower(ws[m]->name) == davi::Lower(*nb)) {
            std::swap(ws[k], ws[m]);
            k = m;
            break;
          }
      break;
    }
  int n = int(items.size());
  index = n == 0 ? 0 : std::max(0, std::min(i, n - 1));
  auto pic = [&](const std::string& comp, int k) {
    if (comp.empty() || !screen.app.skin.assets) return;
    std::string img = k >= 0 && k < n ? items[k].image : "PolE";
    SDL_Surface* s = screen.app.skin.assets->Bitmap(img);
    if (!s) s = screen.app.skin.assets->Bitmap("PolE");
    screen.form.SetPicture(comp, s);
  };
  pic(prev, index - 1);
  pic(cur, n ? index : -1);
  pic(next, index + 1);
  if (!name.empty()) screen.form.SetCaption(name, n ? items[index].name : "");
  if (!info.empty()) screen.form.SetCaption(info, n ? items[index].info : "");
  if (!btn_prev.empty()) screen.form.SetEnabled(btn_prev, index > 0);
  if (!btn_next.empty()) screen.form.SetEnabled(btn_next, index + 1 < n);
}

int Carousel::IndexOf(int id) const {
  for (size_t k = 0; k < items.size(); k++)
    if (items[k].id == id) return int(k);
  return 0;
}

bool Carousel::Click(davi::Widget& w) {
  if (!btn_prev.empty() && Is(w, btn_prev.c_str())) {
    Select(index - 1);
    screen.game.Sound("SNDFX_PHOTO");
    return true;
  }
  if (!btn_next.empty() && Is(w, btn_next.c_str())) {
    Select(index + 1);
    screen.game.Sound("SNDFX_PHOTO");
    return true;
  }
  return false;
}

void ErScreen::OnClick(davi::Widget& w) {
  if (w.kind == davi::Widget::kRadio) {
    if (!w.enabled) return;
    // RadioGroupNumber: one checked per group.
    for (auto& o : form.widgets)
      if (o->kind == davi::Widget::kRadio && o->group == w.group && o->parent == w.parent) o->checked = o.get() == &w;
    game.Sound("SNDFX_RADIO");
    ApplyRadio();
    return;
  }
  if (w.kind == davi::Widget::kCheckBox) return;  // App::Click already toggled it
  if (Is(w, "BtnNext")) {
    game.Sound("SNDFX_OK");
    game.Next();
  } else if (Is(w, "BtnPrevious")) {
    game.Sound("SNDFX_CANCEL");
    game.Previous();
  } else if (Is(w, "BtnQuit")) {
    game.AskQuit();
  } else if (Is(w, "BtnOk") && IsPopup()) {
    game.Sound("SNDFX_OK");
    Close(1);
  } else if (Is(w, "BtnCancel") && IsPopup()) {
    game.Sound("SNDFX_CANCEL");
    Close(0);
  }
}

void ErScreen::Layout() {
  int W = form.w, H = form.h;
  // TBaseFrmCaption 0x491f60: logo at (8, 8); Quit top right, Help left of it.
  davi::Widget *logo = form.Get("LogoImage"), *help = form.Get("BtnHelp"), *quit = form.Get("BtnQuit");
  if (Shown(logo)) Place(logo, 8, 8);
  if (Shown(quit)) {
    Place(quit, W - quit->r.w, 0);
    if (help) Place(help, quit->r.x - help->r.w, 0);
  } else if (help) {
    Place(help, W - help->r.w, 0);
  }
  std::string cls = davi::Lower(form.cls);
  davi::Widget *next = form.Get("BtnNext"), *prev = form.Get("BtnPrevious");
  davi::Widget *ok = form.Get("BtnOk"), *cancel = form.Get("BtnCancel");
  if (cls == "tfrmreqquit") {
    // 0x482234 (replaces the popup layout): 30 px from the right, 20 from the bottom.
    if (Shown(cancel)) {
      Place(cancel, W - cancel->r.w - 30, H - cancel->r.h - 20);
      if (ok) Place(ok, cancel->r.x - ok->r.w - 20, cancel->r.y);
    } else if (ok) {
      Place(ok, W - ok->r.w - 30, H - ok->r.h - 20);
    }
  } else if (next) {
    // TBaseFrmWizard 0x4928b8: Next bottom right, Previous left of it.
    Place(next, W - next->r.w, H - next->r.h);
    if (prev) Place(prev, next->r.x - prev->r.w, next->r.y);
  } else if (ok) {
    // TBaseFrmPopUpWindow 0x48093c: Cancel bottom right, Ok left of it.
    if (Shown(cancel)) {
      Place(cancel, W - cancel->r.w, H - cancel->r.h);
      Place(ok, cancel->r.x - ok->r.w, cancel->r.y);
    } else {
      Place(ok, W - ok->r.w, H - ok->r.h);
    }
  }
}

bool ErScreen::OnKey(SDL_Keycode k) {
  if (k == SDLK_ESCAPE && !IsPopup()) {  // TBaseFrmWizard.FormKeyDown
    if (game.flow.HasPrevious()) game.Previous();
    else game.AskQuit();
    return true;
  }
  return false;
}

davi::Screen* Game::Get(const std::string& cls) {
  std::string key = davi::Lower(cls);
  auto it = screens_.find(key);
  if (it != screens_.end()) return it->second.get();
  std::unique_ptr<davi::Screen> s;
  if (key == "tfrmmainwindow") s = std::make_unique<MainWindow>(*this);
  else if (!(s = MakeQuickRaceScreen(*this, key)) && !(s = MakeChampionshipScreen(*this, key)) &&
           !(s = MakeDialogScreen(*this, key)) && !(s = MakeResultScreen(*this, key)))
    s = std::make_unique<ErScreen>(*this, cls);
  return (screens_[key] = std::move(s)).get();
}

void Game::ShowCurrent() {
  std::string cls = flow.Form();
  if (cls.empty()) return;
  davi::Screen* s = Get(cls);
  if (!s->built) {
    SDL_Log("menu: form %s could not be built", cls.c_str());
    return;
  }
  // Shown when there is a step back, unless the form hides it itself (the
  // result forms set Visible = False in their DFM).
  if (davi::Widget* p = s->form.Get("BtnPrevious")) {
    auto* es = dynamic_cast<ErScreen*>(s);
    p->visible = flow.HasPrevious() && davi::Lower(cls) != "tfrmmainwindow" && (!es || es->PreviousAllowed());
  }
  app.Show(s);
}

void Game::Go(const std::string& state, int step) {
  if (flow.Set(state, step)) ShowCurrent();
}

void Game::RebaseToPlay() {
  const std::string& t = choices.game_type;
  std::string state;
  if (play_mode == 1) {
    if (t == "QR") state = choices.qr_type == 1 ? "FS_QRDUEL" : "FS_QRSTANDARD";  // 0x4940ac
    else if (t == "CH") state = CurrentLeague()->is_new ? "FS_CHNEW" : "FS_CHSTANDARD";
    else if (t == "AD") state = ad.is_new ? "FS_ADNEW" : "FS_ADSTANDARD";
  } else if (play_mode == 2) {
    if (t == "CH") state = "FS_CHWIN";  // 0x4941a4
    else if (t == "AD") state = "FS_ADWIN";
  }
  // TFlowMatrix 0x493a3c: the current form's step in the other state.
  if (!state.empty() && state != flow.State()) flow.Rebase(state);
}

void Game::NextPlain() {
  if (flow.Next()) ShowCurrent();
}

void Game::Next() {
  RebaseToPlay();
  if (flow.Next()) ShowCurrent();
}

void Game::Previous() {
  RebaseToPlay();
  if (flow.Previous()) ShowCurrent();
}

// TFrmMainWindow after a restart with /DEFAULT (0x494218). Quick Race:
// finished -> FS_QRWIN (the winner, whoever it is), else FS_QRESC (back to
// the track choice). FS_QRLOOSE is in GameMenu.ini but never chosen.
void Game::AfterRace() {
  if (choices.game_type == "QR" || choices.game_type.empty()) {
    Go(race.finished ? "FS_QRWIN" : "FS_QRESC");
    return;
  }
  // Championship (CH) / Adventure (AD). Qualified: finished and Position
  // below the race's Qualify-<difficulty> ([CHAMPIONSHIP]/[ADVENTURE]).
  // The last race of the class (CH: 5 per class) or of the adventure (all
  // rows) ends in FS_xxWIN when qualified. 0x494218 also lets game type 5
  // through as qualified; what that type is, is not known (never set here).
  const std::string p = choices.game_type == "AD" ? "FS_AD" : "FS_CH";
  if (!race.finished) {
    Go(p + "ESC");
    return;
  }
  const RaceEntry* e = CurrentRace();
  int d = std::clamp(choices.difficulty, 0, 2);
  bool qualified = e && race.position < e->qualify[d];
  int races = LeagueRaces();
  bool last = CurrentLeague()->round + 1 >= races;
  if (!qualified) Go(p + "DISQUALIFIED");
  else Go(p + (last ? "WIN" : "QUALIFIED"));
}

const RaceEntry* Game::CurrentRace() const {
  if (choices.game_type == "AD") return size_t(ad.round) < data.adventure.size() ? &data.adventure[ad.round] : nullptr;
  if (choices.game_type != "CH") return nullptr;
  int cls = std::clamp(choices.ch_type, 0, 2);
  size_t row = size_t(cls) * 5 + size_t(ch[cls].round);
  return row < data.championship.size() ? &data.championship[row] : nullptr;
}

League* Game::CurrentLeague() {
  if (choices.game_type == "AD") return &ad;
  if (choices.game_type == "CH") return &ch[std::clamp(choices.ch_type, 0, 2)];
  return nullptr;
}

int Game::LeagueRaces() const { return choices.game_type == "AD" ? int(data.adventure.size()) : 5; }

void Game::Ask(const std::string& msg, std::function<void()> yes) {
  davi::Screen* q = Get("TFrmRequester");
  if (!q->built) {
    yes();
    return;
  }
  q->form.SetCaption("TxtContent", msg);
  if (davi::Widget* ok = q->form.Get("BtnOk")) ok->visible = true, ok->caption = texts("LT_YES");
  if (davi::Widget* no = q->form.Get("BtnCancel")) no->visible = true, no->caption = texts("LT_NO");
  app.ShowModal(q, [yes = std::move(yes)](int r) {
    if (r == 1) yes();
  });
}

void Game::AskQuit() {
  davi::Screen* q = Get("TFrmReqQuit");
  if (!q->built) {
    app.Quit(0);
    return;
  }
  // TFrmReqQuit.GDIFormShow 0x482404: "back to the main menu" (BtnRadio1)
  // is checked; OK with BtnRadio2 ends the program.
  q->form.W("BtnRadio1").checked = true;
  q->form.W("BtnRadio2").checked = false;
  app.ShowModal(q, [this, q](int r) {
    if (r != 1) return;
    if (q->form.W("BtnRadio2").checked) app.Quit(0);
    else {
      play_mode = 0;
      Go("FS_DEFAULT");
    }
  });
}

void Game::Sound(const std::string& sfx_id) {
  auto it = data.sfx.find(sfx_id);
  if (it == data.sfx.end() || !app.skin.assets) return;
  app.audio.Play(app.skin.assets->Wav(it->second));
}

void Game::PlayMusic(const std::string& id) {
  auto it = data.music.find(id);
  if (it == data.music.end() || !app.skin.assets) return;
  app.audio.PlayMusic(app.skin.assets->Wav(it->second), 0.6f);
}

}  // namespace er
