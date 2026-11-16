// See switch_intro.h.
//
// The intro was written for ports whose game boots while it plays: it asks
// its host to hold the window and the sound back (RexSwitchHoldOutput) and
// says when it is done (RexSwitchReleaseOutput). This program has nothing to
// boot meanwhile -- the game is up in a moment -- so the hold is simply the
// main thread waiting here, before SDL has opened anything.
#include "switch_intro.h"

#include <atomic>

#include <switch.h>

#include "thorhax_intro.h"

namespace {
std::atomic<bool> g_held{false};
}

extern "C" void RexSwitchHoldOutput(void) { g_held.store(true, std::memory_order_release); }
extern "C" void RexSwitchReleaseOutput(void) { g_held.store(false, std::memory_order_release); }

namespace chaotix {

void play_switch_intro() {
    thorhax::intro::Start();
    while (g_held.load(std::memory_order_acquire)) svcSleepThread(5'000'000);
}

}  // namespace chaotix
