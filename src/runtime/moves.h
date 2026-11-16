// Extra moves for the characters (this fork): moves the game never had, given
// to a character slot by the front end. Off by default, and then the game is
// the original, byte for byte.
//
// Like the ring tether options (runtime/tether.h), a move is applied by the
// patch hook at the start of the characters' frame ($889C10), which the
// interpreter and the recompiled code both call, and depends only on machine
// state and these settings: lockstep, save states and rewind hold.
//
// STOMP (Mighty's drop from Sonic Mania, as the Sonic 3 A.I.R. mod "Ultimate
// Mighty" has it): in a jump, pressing jump again with neither left nor right
// held stops the character dead in the air and sends it straight down as a
// ball, at 9.6 px a frame and faster as gravity adds to it. Landing ends it
// with a short hop: the game's own jump, from a press the hook makes for
// that frame, which the game cuts short because the button is not held.
// Left or right held leaves the press to the game, so Mighty's wall cling
// (jump against a wall) is as it was.
//
// What was measured (chaotix_headless --peek-every, Mighty in Botanic Base 1):
// the player's object is at [FFE034] and the partner's at [FFE036]; object
// byte +7 is the state (02 on the ground, 04 rising in a jump, 08 falling),
// word +$26 the animation ($12 the ball), long +$18 / +$1C the X and Y
// velocity (16.16 px a frame). The buttons a character's code reads are the
// record at FFE04C (player) or FFE05C (partner): +2 held, +3 pressed that
// frame, $20 jump (C); the record is as the code will read it when the hook
// runs.
#pragma once
#include <cstdint>

namespace chaotix {

class Machine;

namespace moves {

// Bits of Machine::move_sets[character].
enum : uint8_t {
    kStomp = 0x01,
};

// Bits of Machine::move_state[who] (0 the player, 1 the partner).
enum : uint8_t {
    kStomping = 0x01,
};

// The stomp's first speed, 16.16 px a frame ($999 in 8.8, the mod's value).
inline constexpr int32_t kStompSpeed = 0x00099900;

// Whether any character has an extra move (a run with one is not a record).
bool any(const Machine& m);

// The hook (called from patches::m68k_hook at tether::kCharsUpdate).
void on_chars_update(Machine& m);

} // namespace moves

} // namespace chaotix
