// The menu's flow matrix (Flow.pas / FlowMatrix.pas in GameMenu.dat): every
// flow state is a list of form classes from GameMenu.ini, shown one after
// the other as a wizard (Next / Previous). [FS_Definition] lists the states;
// [FS_xxx] holds the forms of one state in order.
//
// Example: FS_QRSTANDARD = MainWindow, QRSelectType, QRSelectDifficulty,
// QRSelectTrack, QRSelectCar, Race. After the race the menu switches to
// FS_QRWIN or FS_QRLOOSE, which show the result and go back to QRSelectTrack.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "engine/ini.h"

namespace er {

class FlowMatrix {
 public:
  // Reads [FS_Definition] and every state it names. False (and a log line)
  // if a state is missing, like TFlowMatrix.Init ("Not all states found").
  bool Load(const davi::LayeredIni& ini);

  // Switches to `state` at `step` (0 = its first form).
  bool Set(const std::string& state, int step = 0);
  // Switches state but keeps showing the current form: the original changes
  // state on the main window (radio buttons) before Next is pressed. Finds
  // the current form in the new state; false if it is not in it.
  bool Rebase(const std::string& state);

  const std::string& State() const { return state_; }
  int Step() const { return step_; }
  // Form class of the current step ("" if none).
  std::string Form() const;
  bool HasNext() const;
  bool HasPrevious() const { return step_ > 0; }
  // Moves one step; false at the ends ("There are no next states!").
  bool Next();
  bool Previous();
  // Jumps forward to the next step that shows `form_cls` (SkipWizards).
  bool SkipTo(const std::string& form_cls);

  const std::vector<std::string>* Forms(const std::string& state) const;
  std::vector<std::string> States() const { return order_; }

 private:
  std::map<std::string, std::vector<std::string>> states_;
  std::vector<std::string> order_;
  std::string state_;
  int step_ = 0;
};

}  // namespace er
