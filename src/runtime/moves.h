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
// (jump against a wall) is as it was. As in Sonic Mania, the landing shakes
// the screen and kicks up dust at the feet; both are drawn into the picture
// only (Machine::shake, Machine::stomp_dust; sprite_record.h draws the dust
// from the game's own smoke puffs), so the game runs as without them.
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
// at 12 px a frame (more if it was already moving that way, up to 16), the way
// it faces or the way left or right is held. Letting go before landing drops
// the charge. Ground speed is the object's word +$2C (8.8 px a frame), counted
// forward, the way the character faces: bit 3 of byte +$25 is set while it
// faces left (docs/RING_TETHER.md). The
// game's own sounds are asked for with it: the spin dash's charge ($6C) when
// the dash is charged and its release ($3C) on landing.
//
// RAY'S GLIDE (Sonic Mania Plus, as the Sonic 3 hack "Sonic 3 Unlocked" has it:
// its routine, read from its code, is followed step by step): jump pressed
// again in a jump sets him gliding the way he faces, at 3 px a frame or 7/8 of
// his own speed if that is more, for as long as jump is held. His nose swings
// between down and up, a step a frame. Nose down he falls with all his weight
// and gains speed; nose up he floats and loses it. Forward held (once the nose
// is up) dives; back held (once it is down) pulls up, and a fast dive or the
// first pull throws him upwards by 13/16 of his speed, each pull a little
// weaker. With nothing held at the start he pulls up at once, at half speed.
// It ends on the ground, where he runs on with the speed he had, when jump is
// let go (he drops as a ball), or when anything else takes him (a hit, a
// wall). The glide's five frames (Knuckles' 58-5C) show the nose: 58 all the
// way up, 5A level, 5C all the way down.
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
// ball the jump already is for that time, so it hits what a jump hits, and
// kHammerReach pixels further each way (on_chars_done); the swing's look is
// drawn from the pack's extra frames (extra_frame). The
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
    kGlideDive = 0x08,     // the nose is going down (else up)
    kGlideLeft = 0x10,     // the way it went off
    kGlideBack = 0x20,     // leaning back: the last of left and right held was against the way he goes
};

// Drop dash: frames of holding before it is charged, and its speeds (8.8).
inline constexpr int kDropDashCharge = 20;
inline constexpr int kDropDashSpeed = 0x0C00, kDropDashMax = 0x1000;

// Ray's glide, 8.8 px a frame as its original: the least speed it starts
// with and goes at, the fastest climb and fall, and (16.16) the game's own
// fall in that state.
inline constexpr int kGlideStart = 0x300, kGlideMinSpeed = 0x100, kGlideMaxClimb = 0x600, kGlideMaxFall = 0x1000;
inline constexpr int32_t kGlideOwnFall = 0x100,  // glide_fall_speed 1, as the game adds it
                         kGlideLandMax = 0x0C00;  // ground speed (8.8) kept on landing, at most

// The hammer: how long a swing shows, and its frames in a pack's body sheet
// (kHammerFirst and up, kHammerPoses of them, each shown kHammerHold frames).
inline constexpr int kHammerFrames = 24, kHammerFirst = 0x80, kHammerPoses = 8, kHammerHold = 3;

// The stomp's landing: frames of screen shake, the shake's offsets (picture
// lines, one a frame), and the dust: how long it lasts, in frames.
inline constexpr int kStompShakeFrames = 8;
inline constexpr int8_t kStompShake[kStompShakeFrames] = {4, -4, 3, -3, 2, -2, 1, -1};
inline constexpr int kStompDustFrames = 18;

// The stomp's first speed, 16.16 px a frame ($999 in 8.8, the mod's value).
inline constexpr int32_t kStompSpeed = 0x00099900;

// Whether any character has an extra move (a run with one is not a record).
bool any(const Machine& m);

// The pack's extra body frame to show for the player (who 0) or the partner
// (1), whose body is bank `bank`, in place of the frame the game draws; -1
// for none. Read by the sprite recorder: it changes the picture only.
int extra_frame(const Machine& m, int who, int bank);

// The game's frame goes: the characters ($889C10), then everything else,
// starting at $88804A. What else there is tests for a touch with each
// character's half-size (object bytes +$22, +$23, which the characters' own
// code writes again every frame from the frame shown). Here, with the
// characters done, a swinging hammer makes its character that much larger for
// the rest of the frame: it reaches what the hammer reaches.
inline constexpr uint32_t kCharsDone = 0x88804A;
inline constexpr int kHammerReach = 12;  // pixels, each way
void on_chars_done(Machine& m);

// The hook (called from patches::m68k_hook at tether::kCharsUpdate).
void on_chars_update(Machine& m);

} // namespace moves

} // namespace chaotix
