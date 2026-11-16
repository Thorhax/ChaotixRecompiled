// The Thorhax intro at startup (platforms/switch/thorhax_intro), Switch only.
#pragma once

namespace chaotix {

// Plays the intro and returns when it has ended or been skipped. Call it
// before anything opens the window or the audio device: the intro draws on
// the console's default window and plays through its audio output itself.
void play_switch_intro();

}  // namespace chaotix
