// Europe Racer graphic names. GameMenu.dat leaves every GraphicName empty in
// its forms and sets them in code; these tables are those calls.
#include <map>

#include "engine/dfm.h"
#include "er/game.h"

namespace er {

namespace {

// TData.DataModuleCreate -> 0x4a107c: one call per TGDIButtonDefinition.
// 0x496f8c: prefix + "0" up (also sets the size), "1" down, "2" hovered and
// focused, "3" disabled, "4" focused. 0x4971a8 (the control-key buttons):
// prefix + "0" up and disabled, "1" hovered, down and focused.
struct DefNames {
  const char* prefix;
  bool two_state;
};
const std::map<std::string, DefNames>& Defs() {
  static const std::map<std::string, DefNames> m = {
      {"gdfbtnquit", {"EXT", false}},        {"gdfbtnhelp", {"HLP", false}},
      {"gdfblackimage", {"zwart", false}},   {"gdfbtncnext", {"cbutnx", false}},
      {"gdfbtncprevious", {"cbutp", false}}, {"gdfbtnnext", {"nxt", false}},
      {"gdfbtnprevious", {"prv", false}},    {"gdfbtnok", {"RBut", false}},
      {"gdfbtncancel", {"RBut", false}},     {"gdfbtnradiot1", {"RadM", false}},
      {"gdfbtnradiot2", {"RadD", false}},    {"gdfbtnradiot3", {"RadM", false}},
      {"gdfbtnradiot4", {"RadL", false}},    {"gdfbuttont1", {"BTN", false}},
      {"gdfbuttont2", {"blbut", false}},     {"gdfbuttont3", {"blbutl", false}},
      {"gdftabt1", {"PrTab0", false}},       {"gdftabt2", {"OpTa0", false}},
      {"gdftabt3", {"tabb", false}},         {"gdftabt4", {"Rtab", false}},
      {"gdfopneedle", {"OpSl", false}},      {"gdfcheckshowcontrols", {"CHBX", false}},
      {"gdfbtncontrolkeynormal", {"CoKeN", true}}, {"gdfbtncontrolkeyleft", {"CoKeL", true}},
      {"gdfbtncontrolkeyright", {"CoKeR", true}},  {"gdfbtncontrolkeyup", {"CoKeU", true}},
      {"gdfbtncontrolkeydown", {"CoKeD", true}},   {"gdfbtncontroljoynormal", {"CoJoN", true}},
      {"gdfbtncontroljoyleft", {"CoJoL", true}},   {"gdfbtncontroljoyright", {"CoJoR", true}},
      {"gdfbtncontroljoyup", {"CoJoU", true}},     {"gdfbtncontroljoydown", {"CoJoD", true}},
  };
  return m;
}

// Background per form (TBaseFrmCaption.SetBackground 0x492080, called from
// each form's own override; owner found by the unit the call sits in).
const std::map<std::string, std::string>& Backgrounds() {
  static const std::map<std::string, std::string> m = {
      {"tfrmmainwindow", "BgQR1"},           {"tfrmnameentrynew", "BgNe"},
      {"tfrmnameentrydlg", "WMe"},           {"tfrmrequester", "WMe"},
      {"tfrmreqquit", "WMe"},                {"tfrmplayerlist", "PopPM"},
      {"tfrmplayerlistdlg", "PopPM"},        {"tfrmhighscore", "PopHs"},
      {"tfrminternet", "WIntDlg"},           {"tfrmoptions", "Popop"},
      {"tfrmoptionskeyboard", "BgSet"},      {"tfrmoptionssteer", "BgXSet"},
      {"tfrmoptionssound", "BGGTab"},        {"tfrmoptionsdisplay", "BGGTab"},
      {"tfrmshowcontrols", "BgSet"},         {"tfrmadshowtrack", "BgAdvT"},
      {"tfrmadresultdisqualified", "BgQual"}, {"tfrmadresultqualified", "BgQual"},
      {"tfrmadresultranking", "BGStdna"},    {"tfrmadselectcar", "BgCar"},
      {"tfrmadselectclub", "BgAdvC"},        {"tfrmadselectdifficulty", "BgQR2"},
      {"tfrmadshowstatus", "BgAdvS"},        {"tfrmadgarage", "BgGar"},
      {"tfrmchshowtrack", "BgAdvT"},         {"tfrmchresultdisqualified", "BgQual"},
      {"tfrmchresultqualified", "BgQual"},   {"tfrmchresultranking", "BGStdna"},
      {"tfrmchselectclub", "BgAdvC"},        {"tfrmchselectdifficulty", "BgQR2"},
      {"tfrmchselecttype", "BgE"},           {"tfrmchshowstatus", "BgAdvS"},
      {"tfrmchgarage", "BgGar"}, {"tfrmchresultwin", "zwart0"}, {"tfrmadresultwin", "zwart0"},             {"tfrmqrresultwin", "BgQual"},
      {"tfrmqrselectcar", "BgQRC"},          {"tfrmqrselectdifficulty", "BgQR2"},
      {"tfrmqrselectopponent", "BgQRC2"},    {"tfrmqrselecttrack", "BgQRC2"},
      {"tfrmqrselecttype", "BgE"},           {"tfrmqrresultloose", "BGQRR"},
      {"tfrmchresults", "BGRes"},            {"tfradresults", "BGRes"},
      {"tfrmadresults", "BGRes"},
  };
  return m;
}

}  // namespace

void RegisterSkin(davi::Skin& skin, Texts& texts) {
  davi::Assets* assets = skin.assets;
  skin.def_link = [assets](const std::string& key, int state) -> std::string {
    std::string def = key.substr(key.find('.') + 1);  // "TData.gdfBtnT1" -> "gdfBtnT1"
    auto it = Defs().find(davi::Lower(def));
    if (it == Defs().end()) return "";
    std::string p = it->second.prefix;
    if (it->second.two_state) return p + (state == davi::GraphicDef::kUp || state == davi::GraphicDef::kDisabled ? "0" : "1");
    switch (state) {
      case davi::GraphicDef::kUp: return p + "0";
      case davi::GraphicDef::kDown: return p + "1";
      case davi::GraphicDef::kHovered: return p + "2";
      case davi::GraphicDef::kDisabled: return p + "3";
      case davi::GraphicDef::kFocused:  // "2", overridden by "4" if that exists
        return assets && !assets->Load(p + "4.tga").empty() ? p + "4" : p + "2";
    }
    return "";
  };
  skin.graphic_name = [](const std::string& form, const std::string& comp) -> std::string {
    if (davi::Lower(comp) == "bgndimage") {
      auto it = Backgrounds().find(davi::Lower(form));
      return it == Backgrounds().end() ? "" : it->second;
    }
    return "";
  };
  skin.text = [&texts](const std::string& v) { return texts.Caption(v); };
  // 0x4a15d0: the radio definitions draw their caption 35 px right of the mark.
  for (const char* d : {"gdfBtnRadioT1", "gdfBtnRadioT2", "gdfBtnRadioT3", "gdfBtnRadioT4"}) {
    skin.Def("TData", std::string("Data.") + d);
    auto it = skin.defs.find(std::string("TData.") + d);
    if (it != skin.defs.end()) it->second->caption = {35, 0, 0, 0};
  }
}

}  // namespace er
