#include "er/flow.h"

#include <SDL3/SDL_log.h>

#include "engine/dfm.h"  // Lower

namespace er {

bool FlowMatrix::Load(const davi::LayeredIni& ini) {
  states_.clear();
  order_.clear();
  bool ok = true;
  for (int i = 0; ini.Has("FS_Definition", std::to_string(i)); i++) {
    std::string st = davi::Trim(ini.Get("FS_Definition", std::to_string(i)));
    if (!ini.HasSection(st)) {
      SDL_Log("menu: flow state %s has no [%s] section", st.c_str(), st.c_str());
      ok = false;
      continue;
    }
    std::vector<std::string> forms;
    for (int j = 0; ini.Has(st, std::to_string(j)); j++) forms.push_back(davi::Trim(ini.Get(st, std::to_string(j))));
    states_[davi::Lower(st)] = forms;
    order_.push_back(st);
  }
  return ok && !order_.empty();
}

const std::vector<std::string>* FlowMatrix::Forms(const std::string& state) const {
  auto it = states_.find(davi::Lower(state));
  return it == states_.end() ? nullptr : &it->second;
}

bool FlowMatrix::Set(const std::string& state, int step) {
  const auto* f = Forms(state);
  if (!f || step < 0 || step >= int(f->size())) {
    SDL_Log("menu: no flow step %s[%d]", state.c_str(), step);
    return false;
  }
  state_ = state;
  step_ = step;
  return true;
}

bool FlowMatrix::Rebase(const std::string& state) {
  const auto* f = Forms(state);
  std::string cur = davi::Lower(Form());
  if (!f) return false;
  for (int i = 0; i < int(f->size()); i++)
    if (davi::Lower((*f)[i]) == cur) return Set(state, i);
  return false;
}

std::string FlowMatrix::Form() const {
  const auto* f = Forms(state_);
  return f && step_ < int(f->size()) ? (*f)[step_] : "";
}

bool FlowMatrix::HasNext() const {
  const auto* f = Forms(state_);
  return f && step_ + 1 < int(f->size());
}

bool FlowMatrix::Next() {
  if (!HasNext()) return false;
  step_++;
  return true;
}

bool FlowMatrix::Previous() {
  if (!HasPrevious()) return false;
  step_--;
  return true;
}

bool FlowMatrix::SkipTo(const std::string& form_cls) {
  const auto* f = Forms(state_);
  if (!f) return false;
  for (int i = step_ + 1; i < int(f->size()); i++)
    if (davi::Lower((*f)[i]) == davi::Lower(form_cls)) {
      step_ = i;
      return true;
    }
  return false;
}

}  // namespace er
