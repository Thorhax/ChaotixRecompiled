// A pack's own sounds for the extra moves (runtime/moves.h): played from WAV
// files, looped, while a move asks for them, in place of the game's own sound
// effect for that move.
//
// Each frame a move puts what it wants into Machine::move_sound (one per
// character: the player and the partner); after the frame this player takes
// it, plays that file's share of the frame into the machine's audio output
// and clears it. A sound asked for again carries on where it was, looping
// (from a "smpl" chunk's loop if the file has one, else the whole file); one
// not asked for stops after a few frames (the game lags a frame now and then,
// and its objects, the moves among them, do not run then). The files are the
// host's: the game does not
// hear them, so nothing it does depends on them, but a move leaves out its
// own game sound when Machine::move_sound_files says the file is there.
//
// Files: "tails_flight.wav" (Tails flying) and "tails_flight_tired.wav" (too
// tired to lift), in a pack's Sounds folder or Mods/Sounds.
#pragma once
#include "audio/music_mods.h"
#include <array>
#include <cstdint>
#include <string>

namespace chaotix {
class Machine;
}

namespace chaotix::audio {

class SoundMods {
public:
    // Finds the files in dir and decodes them. A later folder's file takes
    // the place of an earlier one's. Returns how many it holds.
    int scan(const std::string& dir);
    // Sets a sound directly (tests); sound is moves::kSoundFly....
    void set_sound(uint8_t sound, Track track);
    // Tells the machine which sounds there are files for.
    void attach(Machine& m) const;
    // After each emulated frame: mixes the sounds asked for into
    // m.audio_out from first_sample (an index into it, in int16 values) on,
    // at the given level (0-1: the sound effect volume), and clears the asks.
    void after_frame(Machine& m, size_t first_sample, float level);

private:
    struct Voice {
        uint8_t sound = 0;
        size_t pos = 0;  // in frames of the file
        int missed = 0;  // frames in a row it was not asked for
    };
    std::array<Track, 3> files_{};  // by moves::kSoundFly...; empty: none
    std::array<Voice, 2> voices_{};  // the player's and the partner's
};

} // namespace chaotix::audio
