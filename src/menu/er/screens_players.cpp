// Players: the name entry dialogs and the player list (spec section 3 of
// re/menu/players_options_spec.md).
#include <ctime>

#include "er/screens.h"

namespace er {

namespace {

bool Is(const davi::Widget& w, const char* name) { return davi::Lower(w.name) == davi::Lower(name); }

std::string Today() {
  std::time_t t = std::time(nullptr);
  char b[16];
  std::strftime(b, sizeof b, "%Y-%m-%d", std::localtime(&t));
  return b;
}

// "2026-11-09" -> "9-11-2026" (DateToStr with the Dutch short date format).
std::string ShowDate(const std::string& d) {
  int y = 0, m = 0, day = 0;
  if (std::sscanf(d.c_str(), "%d-%d-%d", &y, &m, &day) != 3) return d;
  return std::to_string(day) + "-" + std::to_string(m) + "-" + std::to_string(y);
}

// TFrmNameEntryNew (first player, bg BgNe) / TFrmNameEntryDlg (new player
// from the list, bg WMe): a name of 1-8 characters in EditBox. OK with an
// empty name shows LT_MSG_NONAME and stays.
class NameEntry : public ErScreen {
 public:
  NameEntry(Game& g, const std::string& cls, bool first) : ErScreen(g, cls), first_(first) {
    if (davi::Widget* e = form.Get("EditBox")) e->visible = true, e->max_len = 8;
  }
  std::function<void(const std::string&)> done;
  void OnShow() override {
    ErScreen::OnShow();
    form.SetCaption("TxtTitle", game.texts("LT_APPTITLE"));
    form.SetCaption("TxtContent", first_ ? game.texts("LT_MSG_FIRSTPLAYER") : "");
    form.SetCaption("TxtEditTitle", game.texts("LT_MSG_NEWNAME"));
    if (davi::Widget* e = form.Get("EditBox")) {
      e->caption.clear();
      app.SetFocus(e);
    }
  }
  void OnClick(davi::Widget& w) override {
    if (Is(w, "BtnOk")) {
      std::string name = form.Get("EditBox") ? form.Get("EditBox")->caption : "";
      if (name.empty()) {
        game.Message(game.texts("LT_MSG_NONAME"));
        return;
      }
      auto cb = done;
      Close(1);
      if (cb) cb(name.substr(0, 8));
      return;
    }
    ErScreen::OnClick(w);
  }
  bool OnKey(SDL_Keycode k) override {
    if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
      if (davi::Widget* ok = form.Get("BtnOk")) OnClick(*ok);
      return true;
    }
    return ErScreen::OnKey(k);
  }

 private:
  bool first_;
};

// TFrmPlayerList (0x483658..0x483d70): five rows on a working copy, used
// before empty and newest first; New fills the first empty row (list full:
// LT_MSG_LISTFULL), Delete asks LT_MSG_DELETE. OK applies: deleted slots
// reset, new ones created, the selected row becomes the current player;
// with nobody left LT_MSG_NOPLAYER.
class PlayerList : public ErScreen {
 public:
  PlayerList(Game& g) : ErScreen(g, "TFrmPlayerList") {}
  void OnShow() override {
    ErScreen::OnShow();
    form.SetCaption("TxtTitle", game.texts("LT_TI_PLAYERLIST"));
    form.SetCaption("TxtContent", game.texts("LT_MSG_CHANGEPLAYER"));
    for (int i = 0; i < 5; i++) {
      const Profile& p = game.db.players[i];
      rows_[i] = {p.used ? p.name : "", p.date, !p.used, i, 0};
    }
    selected_ = game.db.current;
    Refresh();
  }
  void OnClick(davi::Widget& w) override {
    for (int k = 0; k < 5; k++)
      if (Is(w, ("btnRadio" + std::to_string(k + 1)).c_str())) {
        if (k < int(order_.size())) selected_ = order_[k];
        game.Sound("SNDFX_RADIO");
        Refresh();
        return;
      }
    if (Is(w, "btnNewPlayer")) {
      int free = -1;
      for (int i = 0; i < 5 && free < 0; i++)
        if (rows_[i].empty) free = i;
      if (free < 0) {
        game.Message(game.texts("LT_MSG_LISTFULL"));
        return;
      }
      game.AskName(false, [this, free](const std::string& name) {
        rows_[free] = {name, Today(), false, free, rows_[free].flags | 2};
        selected_ = free;
        Refresh();
      });
      return;
    }
    if (Is(w, "btnDeletePlayer")) {
      Row& r = rows_[selected_];
      if (r.empty) return;
      std::string msg = game.texts("LT_MSG_DELETE");
      for (int i = 0; i < 2; i++)
        if (size_t p = msg.find("%s"); p != std::string::npos) msg.replace(p, 2, r.name);
      game.Ask(msg, [this] {
        rows_[selected_].empty = true;
        rows_[selected_].flags |= 1;
        for (int i = 0; i < 5; i++)
          if (!rows_[i].empty) {
            selected_ = i;
            break;
          }
        Refresh();
      });
      return;
    }
    if (Is(w, "BtnOk")) {
      bool any = false;
      for (auto& r : rows_) any |= !r.empty;
      if (!any) {
        game.Message(game.texts("LT_MSG_NOPLAYER"));
        return;
      }
      game.StoreProfile();
      for (auto& r : rows_) {
        Profile& p = game.db.players[r.slot];
        if (r.flags & 1) {  // 0x4a2dac: reset (the original keeps bindings and game type)
          int type = p.game_type;
          p = Profile{};
          p.game_type = type;
        }
        if (r.flags & 2) {  // 0x4a2fd4: a new player
          int type = p.game_type;
          p = Profile{};
          p.game_type = type;
          p.used = true;
          p.name = r.name;
          p.date = r.date;
        }
      }
      game.db.current = rows_[selected_].empty ? 0 : selected_;
      game.LoadProfile();
      game.SaveDb();
      Close(1);
      return;
    }
    ErScreen::OnClick(w);
  }

 private:
  struct Row {
    std::string name, date;
    bool empty = true;
    int slot = 0, flags = 0;  // bit0 delete, bit1 create
  };
  // 0x4834d4: used rows first, newest first; empty rows have no caption and
  // a disabled radio.
  void Refresh() {
    order_.clear();
    for (int i = 0; i < 5; i++)
      if (!rows_[i].empty) order_.push_back(i);
    std::stable_sort(order_.begin(), order_.end(), [&](int a, int b) { return rows_[a].date > rows_[b].date; });
    for (int i = 0; i < 5; i++)
      if (rows_[i].empty) order_.push_back(i);
    for (int k = 0; k < 5; k++) {
      const Row& r = rows_[order_[k]];
      std::string radio = "btnRadio" + std::to_string(k + 1);
      if (davi::Widget* b = form.Get(radio)) {
        b->caption = r.empty ? "" : r.name;
        b->enabled = !r.empty;
        b->checked = order_[k] == selected_ && !r.empty;
      }
      form.SetCaption("txtDate" + std::to_string(k + 1), r.empty ? "" : ShowDate(r.date));
    }
  }
  Row rows_[5];
  std::vector<int> order_;
  int selected_ = 0;
};

}  // namespace

std::unique_ptr<davi::Screen> MakePlayerScreen(Game& g, const std::string& key) {
  if (key == "tfrmnameentrynew") return std::make_unique<NameEntry>(g, "TFrmNameEntryNew", true);
  if (key == "tfrmnameentrydlg") return std::make_unique<NameEntry>(g, "TFrmNameEntryDlg", false);
  if (key == "tfrmplayerlist" || key == "tfrmplayerlistdlg") return std::make_unique<PlayerList>(g);
  return nullptr;
}

void Game::AskName(bool first, std::function<void(const std::string&)> done) {
  davi::Screen* s = Get(first ? "TFrmNameEntryNew" : "TFrmNameEntryDlg");
  auto* n = dynamic_cast<NameEntry*>(s);
  if (!n || !s->built) return;
  n->done = std::move(done);
  app.ShowModal(s, [this, first](int r) {
    if (r != 1 && first) app.Quit(0);  // Cancel at the first player ends the program
  });
}

void Game::NewFirstPlayer() {
  // 0x4ab76c: no players yet -> name entry until one exists (slot 0).
  AskName(true, [this](const std::string& name) {
    Profile& p = db.players[0];
    p = Profile{};
    p.used = true;
    p.name = name;
    char b[16];
    std::time_t t = std::time(nullptr);
    std::strftime(b, sizeof b, "%Y-%m-%d", std::localtime(&t));
    p.date = b;
    db.current = 0;
    LoadProfile();
    SaveDb();
  });
}

}  // namespace er
