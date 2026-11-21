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
//
// DROP DASH (Sonic's, from Sonic Mania and Sonic 3 A.I.R.): in a jump, letting
// go of jump and holding it again charges a dash; held for a third of a
// second and still held on landing, the character leaves the ground rolling
// at 8 px a frame (more if it was already moving that way, up to 12), the way
// it faces or the way left or right is held. Letting go before landing drops
// the charge. Ground speed is the object's word +$2C (8.8 px a frame) and
// bit 3 of byte +$25 is set while it faces left (docs/RING_TETHER.md). The
// game's own sounds are asked for with it: the spin dash's charge ($6C) when
// the dash is charged and its release ($3C) on landing.
//
// RAY'S GLIDE (Sonic Mania Plus): jump pressed again in a jump sets him
// gliding the way he faces, at 3 px a frame or his own speed if that is more.
// Forward held (or nothing) he dives: he falls faster and gains speed. Back
// held he pulls up: the speed he has becomes height, and the climb costs
// speed until he levels out and sinks slowly; forward dives again. It ends
// on the ground, where he runs on with the speed he had, or when anything
// else takes him (a hit, a wall). The glide's five frames (Knuckles' 58-5C)
// show how he climbs or falls: 58 a steep climb, 5A level, 5C a steep dive.
//
// It runs in the game's own glide state ($24, Knuckles'), which shows the
// glide's frames and lands; the airborne state ($08) shows the ball whatever
// is written. That state's code ($88AD1C) takes its speeds from the
// character's record, so the character file's [Stats] must make it harmless:
// glide_speed 0x7FFF and glide_acceleration 0 (X is left as written),
// glide_fall_speed 1 (the game's own fall, 1/256 px a frame: it sets the Y
// velocity from it every frame, so the glide's own fall is added to the Y
// position here and kept in Machine::glide_fall), and abilities A without
// bit 6 (or the game starts its own glide from the press). The state ends
// when jump is not held and turns on left or right: the hook holds jump and
// clears the direction word (+8) in the button record while he glides.
//
// HAMMER (Amy's hammer spin, as in Sonic Origins Plus): jump pressed again in
// a jump swings the hammer around her for kHammerFrames frames. She is the
// ball the jump already is for that time, so it hits what a jump hits; the
// swing is its look, drawn from the pack's extra frames (extra_frame). The
// frames left are kept in Machine::glide_fall, which a character with the
// glide uses for the glide and no other does.
#pragma once
#include <cstdint>

namespace chaotix {

class Machine;

namespace moves {

// Bits of Machine::move_sets[character].
enum : uint8_t {
    kStomp = 0x01,
    kDropDash = 0x02,
    kRayGlide = 0x04,  // instead of the other two, not with them
    kHammer = 0x08,    // with the drop dash or alone, not with the glide
};
// A move by the name a character file's [Moves] section gives it; 0 if none.
uint8_t by_name(const char* name);

// Bits of Machine::move_state[who] (0 the player, 1 the partner).
enum : uint8_t {
    kStomping = 0x01,
    kDashReleased = 0x02,  // jump was let go of since the jump began
    kDashCount = 0xFC,     // frames jump has been held again, x 4
    // Ray's glide (a character with it has neither of the moves above).
    kGlideOn = 0x04,
    kGlideUp = 0x08,       // climbing or levelling out, not diving
    kGlideLeft = 0x10,     // the way it went off
};

// Drop dash: frames of holding before it is charged, and its speeds (8.8).
inline constexpr int kDropDashCharge = 20;
inline constexpr int kDropDashSpeed = 0x0C00, kDropDashMax = 0x1000;

// Ray's glide, 16.16 px a frame: the least speed it starts with, the dive's
// pull and push, the climb's pull and drag, and the limits.
inline constexpr int32_t kGlideStart = 0x30000, kGlideDiveFall = 0x2400, kGlideDivePush = 0x1400,
                         kGlideClimbFall = 0x1C00, kGlideClimbDrag = 0x0C00, kGlideMaxSpeed = 0xC0000,
                         kGlideMinSpeed = 0x10000, kGlideMaxFall = 0x80000, kGlideSink = 0x20000,
                         kGlideOwnFall = 0x100,  // glide_fall_speed 1, as the game adds it
                         kGlideLandMax = 0x0C00;  // ground speed (8.8) kept on landing, at most

// The hammer: how long a swing shows, and its frames in a pack's body sheet
// (kHammerFirst and up, kHammerPoses of them, each shown kHammerHold frames).
inline constexpr int kHammerFrames = 24, kHammerFirst = 0x80, kHammerPoses = 8, kHammerHold = 3;

// The stomp's first speed, 16.16 px a frame ($999 in 8.8, the mod's value).
inline constexpr int32_t kStompSpeed = 0x00099900;

// Whether any character has an extra move (a run with one is not a record).
bool any(const Machine& m);

// The pack's extra body frame to show for the player (who 0) or the partner
// (1), whose body is bank `bank`, in place of the frame the game draws; -1
// for none. Read by the sprite recorder: it changes the picture only.
int extra_frame(const Machine& m, int who, int bank);

// The hook (called from patches::m68k_hook at tether::kCharsUpdate).
void on_chars_update(Machine& m);

} // namespace moves

} // namespace chaotix
