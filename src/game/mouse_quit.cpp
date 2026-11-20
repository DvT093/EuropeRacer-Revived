// handle_mouse (0x50B3E0, SCIPIO Win95Lib Mouse.cpp): the message pump
// 0x505F30 calls it whenever the queue is empty. When the race window is
// closed, the game tears down DirectDraw and the mouse, posts WM_CLOSE to
// its (already destroyed) window and pumps once more: nothing arrives, so
// handle_mouse runs on the torn-down mouse and the game shows
// "Mouse.cpp(772) : (handle_mouse) Mouse is not initialised" before it
// exits. Without a mouse there is nothing to handle: return quietly.

#include "x86/runtime.h"

namespace recomp::gen {

void f_0050B3E0_lifted(::recomp::Ctx& c);

void f_0050B3E0(::recomp::Ctx& c) {
  if (Rd32(0x6FB6DC) == 0) {  // the mouse object: 0 = not initialised
    c.eax = 0;
    c.esp += 4;  // ret
    return;
  }
  f_0050B3E0_lifted(c);
}

}  // namespace recomp::gen
