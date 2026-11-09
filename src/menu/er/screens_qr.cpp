// Quick Race screens: type, difficulty, track, car, opponent.
//
// The original builds each screen from small controller objects (GameMenu.dat):
// a radio group (0x48aa78), a text per radio into TxtInfo (0x4aafc8), a
// picture per radio (0x48af24) and a carousel of three polaroids with
// previous/next buttons (base 0x4b893c, e.g. tracks 0x4c38d0). The texts are
// TLanguageText enum values; the names are in re/menu/lt_enum.txt.
#include <cstdio>
#include <cstdlib>

#include "engine/dfm.h"
#include "er/screens.h"

namespace er {

namespace {

bool Is(const davi::Widget& w, const char* name) { return davi::Lower(w.name) == davi::Lower(name); }

std::string Fmt2(const char* fmt, int v) {
  char b[64];
  std::snprintf(b, sizeof b, fmt, v);
  return b;
}

// TFrmQRSelectType 0x4c4860: four race types, info text per type.
class QRSelectType : public ErScreen {
 public:
  QRSelectType(Game& g) : ErScreen(g, "TFrmQRSelectType") {
    BindRadios({"LT_INF_QR_TRACK", "LT_INF_QR_DUEL", "LT_INF_QR_TIMETRIAL", "LT_INF_QR_SUDDENDEATH"}, {});
  }
  void OnClick(davi::Widget& w) override {
    if (Is(w, "BtnNext")) {
      game.choices.qr_type = CheckedRadio();
      // A duel has its own flow with the opponent screen (FS_QRDUEL).
      game.flow.Rebase(game.choices.qr_type == 1 ? "FS_QRDUEL" : "FS_QRSTANDARD");
    }
    ErScreen::OnClick(w);
  }
};

// TFrmQRSelectDifficulty 0x4c26c4: picture dfimg0-2 and an info text per level.
class QRSelectDifficulty : public ErScreen {
 public:
  QRSelectDifficulty(Game& g) : ErScreen(g, "TFrmQRSelectDifficulty") {
    form.SetCaption("TxtContent", game.texts("LT_MSG_QRDIFFICULTY"));  // text 0xb
    BindRadios({"LT_INF_DF_EASY", "LT_INF_DF_NORMAL", "LT_INF_DF_HARD"}, {"dfimg0", "dfimg1", "dfimg2"},
               "ImgDifficulty");
    SetRadio(game.choices.difficulty);
  }
  void OnClick(davi::Widget& w) override {
    ErScreen::OnClick(w);
    game.choices.difficulty = CheckedRadio();
  }
};

// TFrmQRSelectTrack 0x4c42a0: a region per radio (West, Middle, South,
// downloaded = locked), the region's tracks as a carousel (trk<id>A, PolE
// when there is none), the track name under it and the track info.
// 0x4c3ce0 lists the region's tracks under each radio in Arial 7.
class QRSelectTrack : public ErScreen {
 public:
  QRSelectTrack(Game& g) : ErScreen(g, "TFrmQRSelectTrack"), car_(*this) {
    form.SetCaption("TxtContent", Format(game.texts("LT_MSG_TRACK")));  // text 0xe
    car_.prev = "ImgTrackPrev", car_.cur = "ImgTrackCur", car_.next = "ImgTrackNext";
    car_.name = "lblTrackName", car_.info = "TxtInfo";
    car_.btn_prev = "BtnTrackPrev", car_.btn_next = "BtnTrackNext";
    const char* regions[] = {"TC_WESTEUROPE", "TC_MIDDLEEUROPE", "TC_SOUTHEUROPE"};
    const char* lists[] = {"lblTracksWest", "lblTracksMiddle", "lblTracksSouth"};
    for (int r = 0; r < 3; r++) {
      std::string names;
      for (auto& q : game.data.quickrace)
        if (const Track* t = game.data.TrackById(q.track); t && t->region == regions[r])
          names += (names.empty() ? "" : ", ") + game.texts(t->name);
      if (davi::Widget* l = form.Get(lists[r])) {
        l->caption = names;
        l->font.font = 2;
        l->font.px = 9;  // Arial 7 pt
        l->font.color = {0, 0, 0, 255};
        l->wrap = true;
        l->autosize = false;
      }
    }
    // Downloaded tracks: none in this port, so locked like on a fresh install.
    form.SetEnabled("BtnRadio4", false);
    form.SetVisible("TxtRadio4Sub", true);
    SetRadio(region_);
    Fill();
  }
  void Layout() override {
    ErScreen::Layout();
    const char* radios[] = {"BtnRadio1", "BtnRadio2", "BtnRadio3"};
    const char* lists[] = {"lblTracksWest", "lblTracksMiddle", "lblTracksSouth"};
    for (int r = 0; r < 3; r++)
      if (davi::Widget *rb = form.Get(radios[r]), *l = form.Get(lists[r]); rb && l)
        l->r = {rb->r.x + 37, rb->r.y + 30, 260, 28};
    if (davi::Widget *rb = form.Get("BtnRadio4"), *s = form.Get("TxtRadio4Sub"); rb && s)
      s->r.x = rb->r.x + 37, s->r.w = 300;
  }
  void OnClick(davi::Widget& w) override {
    if (car_.Click(w)) {
      game.choices.track = car_.Current();
      return;
    }
    ErScreen::OnClick(w);
    if (w.kind == davi::Widget::kRadio) {
      region_ = CheckedRadio();
      Fill();
    }
  }

 private:
  void Fill() {
    const char* regions[] = {"TC_WESTEUROPE", "TC_MIDDLEEUROPE", "TC_SOUTHEUROPE"};
    car_.items.clear();
    for (auto& q : game.data.quickrace)
      if (const Track* t = game.data.TrackById(q.track); t && region_ < 3 && t->region == regions[region_])
        car_.items.push_back({t->id, Fmt2("trk%.2dA", t->id), game.texts(t->name), game.texts(t->qr_info)});
    car_.Select(car_.IndexOf(game.choices.track));
    game.choices.track = car_.Current();
  }
  Carousel car_;
  int region_ = 0;
};

// TFrmQRSelectCar 0x4c2138: a car class per radio (standard, ATV, sport,
// downloaded = locked), the class's cars as a carousel (car<id>A), the car
// text and its properties as bars on the MensMal note (ImgProperties).
class QRSelectCar : public ErScreen {
 public:
  QRSelectCar(Game& g) : ErScreen(g, "TFrmQRSelectCar"), car_(*this) {
    form.SetCaption("TxtContent", game.texts("LT_MSG_QRCAR"));  // text 0xd
    car_.prev = "ImgCarPrev", car_.cur = "ImgCarCur", car_.next = "ImgCarNext";
    car_.info = "TxtInfo";
    car_.btn_prev = "BtnCarPrev", car_.btn_next = "BtnCarNext";
    if (app.skin.assets) form.SetPicture("ImgProperties", app.skin.assets->Bitmap("MensMal"));
    form.SetEnabled("BtnRadio4", false);
    form.SetVisible("TxtRadio4Sub", true);
    SetRadio(class_);
    Fill();
  }
  void Layout() override {
    ErScreen::Layout();
    if (davi::Widget *rb = form.Get("BtnRadio4"), *s = form.Get("TxtRadio4Sub"); rb && s)
      s->r.x = rb->r.x + 37, s->r.w = 300;
  }
  void OnClick(davi::Widget& w) override {
    if (car_.Click(w)) {
      game.choices.car = car_.Current();
      return;
    }
    ErScreen::OnClick(w);
    if (w.kind == davi::Widget::kRadio) {
      class_ = CheckedRadio();
      Fill();
    }
  }
  // TGDIMultiProgressBar MpbCarProperties: a label and 10 bar images
  // (stats1 filled, stats0 empty) per property, values from [CARS] at the
  // Quick Race upgrade stage ([TWEAK] QR_UPGRADE). Row layout measured from
  // the DFM offsets (5 px), not yet compared with the original.
  void PaintOver(SDL_Surface* canvas, int ox, int oy) override {
    davi::Widget* mpb = form.Get("MpbCarProperties");
    const Car* c = game.data.CarById(car_.Current());
    if (!mpb || !c || !app.skin.assets) return;
    SDL_Surface* on = app.skin.assets->Bitmap("Stats1");
    SDL_Surface* off = app.skin.assets->Bitmap("Stats0");
    int stage = game.data.qr_upgrade == "US_NONE" ? 0 : game.data.qr_upgrade == "US_STAGE1" ? 1
                : game.data.qr_upgrade == "US_STAGE2" ? 2 : 3;
    const char* labels[] = {"LT_CPROP_SPEED", "LT_CPROP_ACCELERATION", "LT_CPROP_GRIP", "LT_CPROP_STRENGTH"};
    davi::Font* f = app.skin.GetFont(4);
    for (int p = 0; p < 4; p++) {
      int y = oy + mpb->r.y + 5 + p * 19;
      int v = size_t(stage * 4 + p) < c->props.size() ? c->props[stage * 4 + p] / 10 : 0;
      if (f) f->DrawLine(canvas, ox + mpb->r.x + 5, y, game.texts(labels[p]), 11, {0, 0, 0, 255});
      for (int i = 0; i < 10; i++) {
        SDL_Surface* s = i < v ? on : off;
        if (!s) continue;
        SDL_Rect d{ox + mpb->r.x + 95 + i * s->w, y, s->w, s->h};
        SDL_BlitSurface(s, nullptr, canvas, &d);
      }
    }
  }

 private:
  void Fill() {
    const char* classes[] = {"CT_STANDARD", "CT_ATV", "CT_SPORT"};
    car_.items.clear();
    for (auto& c : game.data.cars)
      if (class_ < 3 && c.car_type == classes[class_])
        car_.items.push_back({c.id, Fmt2("car%.2dA", c.id), "", game.texts(c.info)});
    car_.Select(car_.IndexOf(game.choices.car));
    game.choices.car = car_.Current();
  }
  Carousel car_;
  int class_ = 0;
};

// TFrmQRSelectOpponent 0x4c310c: the eight club racers (cbimgdo<club>) or
// J.B. Black, the police chief (cbimgdo8), with name and info.
class QRSelectOpponent : public ErScreen {
 public:
  QRSelectOpponent(Game& g) : ErScreen(g, "TFrmQRSelectOpponent"), car_(*this) {
    form.SetCaption("TxtContent", Format(game.texts("LT_MSG_DUEL")));  // text 0xf
    car_.prev = "ImgRacerPrev", car_.cur = "ImgRacerCur", car_.next = "ImgRacerNext";
    car_.info = "TxtInfo";
    car_.btn_prev = "BtnRacerPrev", car_.btn_next = "BtnRacerNext";
    SetRadio(0);
    Fill();
  }
  void OnClick(davi::Widget& w) override {
    if (car_.Click(w)) {
      game.choices.opponent = car_.Current();
      return;
    }
    ErScreen::OnClick(w);
    if (w.kind == davi::Widget::kRadio) Fill();
  }

 private:
  void Fill() {
    car_.items.clear();
    if (CheckedRadio() == 1) {
      car_.items.push_back({8, "cbimgdo8", game.texts("LT_ED_CHIEFPOLICE"), game.texts("LT_INF_ED_CHIEFPOLICE")});
    } else {
      for (size_t i = 0; i < game.data.clubs.size(); i++)
        car_.items.push_back({int(i), Fmt2("cbimgdo%d", int(i)), game.texts(game.data.clubs[i].opponent),
                              game.texts(game.data.clubs[i].opponent_info)});
    }
    car_.Select(car_.IndexOf(game.choices.opponent));
    game.choices.opponent = car_.Current();
  }
  Carousel car_;
};

// TFrmRace 0x4b4558: writes game.ini and leaves for the race (the original
// quits; the launcher runs the game and restarts the menu with /DEFAULT).
// The race is not ported yet. For tests ERACER_RACE_RESULT=<ini> stands in
// for it: that file is read as the game.ini the race left behind.
class Race : public ErScreen {
 public:
  Race(Game& g) : ErScreen(g, "TFrmRace") {}
  void OnShow() override {
    ErScreen::OnShow();
    // 0x4b4558: a new championship gets its opponents before the first race.
    if (League* l = game.CurrentLeague()) {
      l->player_club = game.choices.club;
      if (l->is_new) l->PickOpponents();
      l->is_new = false;
    }
    game.race_counted = false;
    const char* fake = std::getenv("ERACER_RACE_RESULT");
    if (!fake) {
      SDL_Log("menu: the race is not ported yet");
      return;
    }
    // After this show has finished (AfterRace shows another form).
    app.After(0, [this, path = std::string(fake)] {
      davi::IniFile ini;
      ini.Load(path);
      if (!game.race.Load(ini)) SDL_Log("menu: race result %s not found", path.c_str());
      // The race writes back the country<ii> the menu gave it (0x4988ec):
      // the league's opponents. A test file cannot know those (random).
      if (League* l = game.CurrentLeague())
        for (size_t i = 0; i < game.race.racers.size() && i < 4; i++) game.race.racers[i].club = l->opponents[i];
      game.AfterRace();
    }, this);
  }
};

}  // namespace

std::unique_ptr<davi::Screen> MakeQuickRaceScreen(Game& g, const std::string& key) {
  if (key == "tfrmqrselecttype") return std::make_unique<QRSelectType>(g);
  if (key == "tfrmqrselectdifficulty") return std::make_unique<QRSelectDifficulty>(g);
  if (key == "tfrmqrselecttrack") return std::make_unique<QRSelectTrack>(g);
  if (key == "tfrmqrselectcar") return std::make_unique<QRSelectCar>(g);
  if (key == "tfrmqrselectopponent") return std::make_unique<QRSelectOpponent>(g);
  if (key == "tfrmrace") return std::make_unique<Race>(g);
  return nullptr;
}

}  // namespace er
