// Extra moves for the characters. See moves.h.
#include "runtime/moves.h"
#include "runtime/log.h"
#include "runtime/patches.h"
#include "runtime/system.h"
#include "runtime/tether.h"

#include <algorithm>
#include <string>

namespace chaotix::moves {

namespace {

uint16_t rd16(const Machine& m, uint32_t off) {
    off &= 0xFFFF;
    return uint16_t(m.wram[off] << 8 | m.wram[(off + 1) & 0xFFFF]);
}
uint32_t rd32(const Machine& m, uint32_t off) { return uint32_t(rd16(m, off)) << 16 | rd16(m, off + 2); }
void wr32(Machine& m, uint32_t off, uint32_t v) {
    for (int i = 0; i < 4; ++i) m.wram[(off + uint32_t(i)) & 0xFFFF] = uint8_t(v >> (24 - 8 * i));
}

constexpr uint16_t kModeLevel = 0x18;
constexpr uint32_t kCharacterId[2] = {0xE038, 0xE03A};  // word: character x 4

// Object fields (docs/CHARACTERS.md section 3).
constexpr uint32_t kObjState = 0x07;
constexpr uint32_t kObjXVel = 0x18, kObjYVel = 0x1C;  // 16.16
constexpr uint32_t kObjAnim = 0x26;
constexpr uint32_t kObjHalfWidth = 0x22, kObjHalfHeight = 0x23;  // bytes, pixels
constexpr uint32_t kObjX = 0x08;  // word, pixels
constexpr uint32_t kObjY = 0x0C;  // long, 16.16 (its first word: pixels)
constexpr uint8_t kStateGround = 0x02, kStateJump = 0x04, kStateRoll = 0x06, kStateFall = 0x08;
constexpr uint16_t kAnimBall = 0x12;

// The button record: +2 held, +3 pressed this frame.
constexpr uint8_t kJump = 0x20;
constexpr uint8_t kLeftRight = 0x0C;

// A sound effect, asked for as the game's own routine does ($8F6E76): the
// id into the first free of the Z80 driver's three request bytes.
constexpr uint32_t kSoundQueue = 0x1C0A;
constexpr uint8_t kSoundDashCharge = 0x6C, kSoundDashRelease = 0x3C;
void post_sound(Machine& m, uint8_t id) {
    for (uint32_t i = 0; i < 3; ++i)
        if (!m.zram[kSoundQueue + i]) { m.zram[kSoundQueue + i] = id; return; }
}

void stomp(Machine& m, int who, uint32_t obj, uint32_t pad) {
    uint8_t& now = m.move_state[who];
    const uint8_t state = m.wram[(obj + kObjState) & 0xFFFF];
    const bool in_air = state == kStateJump || state == kStateFall;
    if (now & kStomping) {
        if (in_air) {
            // Something sent it back up (an enemy, a spring): the stomp is over.
            if (int32_t(rd32(m, obj + kObjYVel)) < kStompSpeed / 2) {
                now &= uint8_t(~kStomping);
                return;
            }
            wr32(m, obj + kObjXVel, 0);
            return;
        }
        now &= uint8_t(~kStomping);
        // Landed: the hop. A press the game sees this frame only.
        if (state == kStateGround || state == kStateRoll) {
            m.wram[(pad + 3) & 0xFFFF] |= kJump;
            // And, as in Sonic Mania, the screen shakes and dust flies at the
            // feet (both only in the picture).
            m.shake = kStompShakeFrames;
            m.stomp_dust = Machine::StompDust{true, m.frame_count, int16_t(rd16(m, obj + kObjX)),
                                              int16_t(rd16(m, obj + kObjY)) + m.wram[(obj + kObjHalfHeight) & 0xFFFF], 0};
        }
        return;
    }
    if (!in_air || rd16(m, obj + kObjAnim) != kAnimBall) return;
    const uint8_t held = m.wram[(pad + 2) & 0xFFFF];
    uint8_t& pressed = m.wram[(pad + 3) & 0xFFFF];
    if (!(pressed & kJump) || (held & kLeftRight)) return;
    pressed &= uint8_t(~kJump);  // the press is the stomp's, not the game's
    wr32(m, obj + kObjXVel, 0);
    wr32(m, obj + kObjYVel, uint32_t(kStompSpeed));
    m.wram[(obj + kObjState) & 0xFFFF] = kStateFall;
    now |= kStomping;
    LOGD("moves", "stomp: character %d", who);
}

void wr16(Machine& m, uint32_t off, uint16_t v) {
    m.wram[off & 0xFFFF] = uint8_t(v >> 8);
    m.wram[(off + 1) & 0xFFFF] = uint8_t(v);
}

constexpr uint32_t kObjFlags = 0x25;  // bit 3: facing left
constexpr uint32_t kObjGroundSpeed = 0x2C;  // 8.8

void drop_dash(Machine& m, int who, uint32_t obj, uint32_t pad) {
    uint8_t& now = m.move_state[who];
    const uint8_t state = m.wram[(obj + kObjState) & 0xFFFF];
    const uint8_t held = m.wram[(pad + 2) & 0xFFFF];
    const bool in_air = state == kStateJump || state == kStateFall;
    const int count = (now & kDashCount) >> 2;
    if (in_air && rd16(m, obj + kObjAnim) == kAnimBall && !(now & kStomping)) {
        if (!(held & kJump)) {
            now = uint8_t((now & ~kDashCount) | kDashReleased);  // let go: the charge is lost
        } else if ((now & kDashReleased) && count < kDropDashCharge) {
            now = uint8_t((now & ~kDashCount) | ((count + 1) << 2));
            if (count + 1 == kDropDashCharge) post_sound(m, kSoundDashCharge);
        }
        return;
    }
    now &= uint8_t(~(kDashReleased | kDashCount));
    if (count < kDropDashCharge || !(held & kJump) || (state != kStateGround && state != kStateRoll)) return;
    // Landed charged: off it goes, rolling.
    bool left = (m.wram[(obj + kObjFlags) & 0xFFFF] & 0x08) != 0;
    if (held & 0x04) left = true;
    if (held & 0x08) left = false;
    // Ground speed counts forward, the way the character faces (not to the
    // right): speed the way it faced at landing is kept if it goes on that way.
    uint8_t& flags = m.wram[(obj + kObjFlags) & 0xFFFF];
    const bool faced_left = (flags & 0x08) != 0;
    const int had = int16_t(rd16(m, obj + kObjGroundSpeed));
    const int along = faced_left == left ? had : -had;
    int speed = kDropDashSpeed;
    if (along > 0) speed = std::min(kDropDashSpeed + along / 4, kDropDashMax);
    LOGD("moves", "drop dash: character %d, speed %d %s", who, speed, left ? "left" : "right");
    wr16(m, obj + kObjGroundSpeed, uint16_t(speed));
    wr32(m, obj + kObjXVel, uint32_t(int32_t(left ? -speed : speed) * 256));
    flags = uint8_t(left ? flags | 0x08 : flags & ~0x08);
    // As in Sonic Mania, dust is left behind where it set off (picture only).
    m.stomp_dust = Machine::StompDust{true, m.frame_count, int16_t(rd16(m, obj + kObjX)),
                                      int16_t(rd16(m, obj + kObjY)) + m.wram[(obj + kObjHalfHeight) & 0xFFFF],
                                      int8_t(left ? 1 : -1)};
    m.wram[(obj + kObjState) & 0xFFFF] = kStateRoll;
    wr16(m, obj + kObjAnim, kAnimBall);
    post_sound(m, kSoundDashRelease);
}

constexpr uint16_t kAnimGlide = 0x40;
constexpr uint8_t kStateGlide = 0x24;
constexpr uint32_t kObjAnimFrame = 0x28;  // 8.8

// 256 x the sine of 0, 4, ... 64 of the 256 steps in a circle, as the game the glide is from has it.
constexpr int16_t kSine[17] = {0, 25, 49, 74, 97, 120, 142, 162, 181, 197, 212, 225, 236, 244, 251, 254, 256};
// The sine and cosine of an angle that is a multiple of 4 steps, -64 to 64.
int sine(int a) { return a < 0 ? -kSine[-a / 4] : kSine[a / 4]; }
int cosine(int a) { return kSine[(64 - (a < 0 ? -a : a)) / 4]; }

void ray_glide(Machine& m, int who, uint32_t obj, uint32_t pad) {
    uint8_t& now = m.move_state[who];
    Machine::Glide& g = m.glide[who];
    int32_t& fall = m.glide_fall[who];  // 16.16; the routine below works in 8.8 like its original
    uint8_t& state = m.wram[(obj + kObjState) & 0xFFFF];
    uint8_t& held = m.wram[(pad + 2) & 0xFFFF];
    uint8_t& flags = m.wram[(obj + kObjFlags) & 0xFFFF];
    // Left or right held sets which way he leans: back (up) or forward (down).
    const auto lean = [&](bool left) {
        if (!(held & kLeftRight)) return;
        const bool forward = (held & (left ? 0x04 : 0x08)) != 0;
        now = uint8_t(forward ? now & ~kGlideBack : now | kGlideBack);
    };
    // The climb a pull gives: 13/16 of his speed, upwards.
    const auto pull = [](int xv) {
        const int d = xv < 0 ? xv : -xv;
        return (d >> 1) + (d >> 2) + (d >> 4);
    };
    if (!(now & kGlideOn)) {
        now = 0;
        uint8_t& pressed = m.wram[(pad + 3) & 0xFFFF];
        const bool in_air = state == kStateJump || state == kStateFall;
        if (!in_air || rd16(m, obj + kObjAnim) != kAnimBall || !(pressed & kJump)) return;
        pressed &= uint8_t(~kJump);
        const bool left = (flags & 0x08) != 0;
        // It starts leaning back and diving; forward held starts the dive
        // as it is, anything else pulls up at once.
        now = uint8_t(kGlideOn | kGlideDive | kGlideBack | (left ? kGlideLeft : 0));
        int xv = int32_t(rd32(m, obj + kObjXVel)) >> 8;
        xv -= xv >> 3;
        if ((left ? -xv : xv) < kGlideStart) xv = left ? -kGlideStart : kGlideStart;
        lean(left);
        g = Machine::Glide{};
        if (now & kGlideBack) {
            now &= uint8_t(~kGlideDive);
            xv >>= 1;
            g.impulse = int16_t(pull(xv));
        }
        fall = (int32_t(rd32(m, obj + kObjYVel)) >> 1) & ~0xFF;
        g.pitch = 0x40;
        g.boost = 0x40;
        g.speed = int16_t(xv < 0 ? -xv : xv);
        wr32(m, obj + kObjXVel, uint32_t(xv * 256));
        state = kStateGlide;
        wr16(m, obj + kObjAnim, kAnimGlide);
        LOGD("moves", "glide: character %d", who);
    } else if (state != kStateGlide) {
        // The ground, a hit, a wall: over. That state's code takes hold of a
        // wall as Knuckles does ($26, $28): Ray does not climb, he drops.
        now = 0;
        if (state == kStateGround || state == kStateRoll) {
            // The state's landing leaves him standing (ground speed 0) with
            // the X velocity still as it was: he runs on with what he had,
            // facing the way he went (ground speed counts forward).
            const int32_t v = std::clamp(int32_t(rd32(m, obj + kObjXVel)) / 0x100, -kGlideLandMax, kGlideLandMax);
            if (rd16(m, obj + kObjGroundSpeed) == 0 && v) {
                wr16(m, obj + kObjGroundSpeed, uint16_t(int16_t(v < 0 ? -v : v)));
                flags = uint8_t(v < 0 ? flags | 0x08 : flags & ~0x08);
            }
        }
        if (state == 0x26 || state == 0x28) {
            state = kStateFall;
            wr32(m, obj + kObjXVel, 0);
            wr32(m, obj + kObjYVel, 0);
        }
        return;
    } else {
        const bool left = (now & kGlideLeft) != 0;
        int xv = int32_t(rd32(m, obj + kObjXVel)) >> 8;
        int yv = fall >> 8;
        // Stopped dead by something in the way, or jump let go: he drops.
        if ((left ? -xv : xv) < kGlideMinSpeed / 2 || !(held & kJump)) {
            now = 0;
            state = kStateFall;
            wr16(m, obj + kObjAnim, kAnimBall);
            wr32(m, obj + kObjYVel, uint32_t(fall));
            return;
        }
        // The nose: down to 0x10 in a dive, up to 0x70 out of one.
        if (now & kGlideDive) { if (g.pitch > 0x10) g.pitch -= 8; }
        else if (g.pitch < 0x70) g.pitch += 8;
        if (g.impulse) {
            // A pull's climb comes on over a few frames.
            yv += g.impulse >> 2;
            if (g.impulse > yv) { yv = g.impulse; g.impulse = 0; }
        } else {
            // His weight: all of it nose down, little of it nose up.
            yv += (cosine(g.pitch / 2) * 2 * 0x38) >> 9;
        }
        yv = std::max(yv, -kGlideMaxClimb);
        if (!(now & kGlideDive) && yv > 0x100) yv -= yv >> 2;
        yv = std::min(yv, kGlideMaxFall);
        // Forward: gained nose down, lost nose up, and never more than the
        // speed he has in store, which the fall of a dive fills.
        const int push = (sine(0x50 - g.pitch) * 0x16) >> 8;
        int store = g.speed;
        if (yv <= 0) {
            g.speed = int16_t(g.speed - push);
            if (store <= 0x400) g.speed = int16_t(store = 0x400);
        } else if (store < yv) {
            g.speed = int16_t(store = yv - (yv >> 6));
        }
        if (xv) {
            if (left) xv = std::max(std::min(xv - push, -kGlideMinSpeed), -store);
            else xv = std::min(std::max(xv + push, int(kGlideMinSpeed)), store);
        }
        lean(left);
        if (!(now & kGlideBack)) {
            // Forward, once the nose is all the way up: the dive.
            if (g.pitch == 0x70 && !(now & kGlideDive)) {
                now |= kGlideDive;
                g.impulse = 0;
            }
        } else if (g.pitch == 0x10 && (now & kGlideDive)) {
            // Back, once the nose is all the way down: he pulls up, and a
            // dive fast enough (or the first pull) throws him upwards, each
            // time a little less.
            now &= uint8_t(~kGlideDive);
            if (yv > 0x280 || g.boost == 0x40) {
                g.impulse = int16_t(std::max((pull(xv) * (g.boost * 4)) >> 8, -kGlideMaxClimb));
                if (g.boost > 4) g.boost -= 8;
            }
        }
        fall = yv * 256;
        wr32(m, obj + kObjXVel, uint32_t(xv * 256));
        flags = uint8_t(left ? flags | 0x08 : flags & ~0x08);
    }
    // The pose: the glide's animation ($40) has five frames (Knuckles' 58-5C,
    // which his own glide runs through once); here the frame says how the
    // nose points, from all the way up to all the way down. The game's step
    // adds a quarter of a frame to what is written, which stays in the frame.
    const int pose = g.pitch >= 0x70 ? 0 : g.pitch >= 0x58 ? 1 : g.pitch >= 0x40 ? 2 : g.pitch >= 0x28 ? 3 : 4;
    wr16(m, obj + kObjAnimFrame, uint16_t(pose << 8));
    // The state's code moves him by the X velocity and by its own small fall:
    // the rest of the fall is added to where he is.
    wr32(m, obj + kObjY, rd32(m, obj + kObjY) + uint32_t(fall - kGlideOwnFall));
    // What that code reads: jump held (or it drops him), no direction (or it turns him).
    held = uint8_t((held | kJump) & ~kLeftRight);
    wr16(m, pad + 8, 0);
}

void hammer(Machine& m, int who, uint32_t obj, uint32_t pad) {
    int32_t& left = m.glide_fall[who];
    const uint8_t state = m.wram[(obj + kObjState) & 0xFFFF];
    const bool in_air = state == kStateJump || state == kStateFall;
    if (!in_air || rd16(m, obj + kObjAnim) != kAnimBall) {
        left = 0;
    } else if (left > 0) {
        --left;
    } else if (m.wram[(pad + 3) & 0xFFFF] & kJump) {
        left = kHammerFrames;
        LOGD("moves", "hammer: character %d", who);
    }
}

} // namespace

void on_chars_done(Machine& m) {
    if (rd16(m, patches::kGameMode) != kModeLevel) return;
    const uint32_t objs[2] = {rd16(m, tether::kPlayerObj), rd16(m, tether::kPartnerObj)};
    for (int who = 0; who < 2; ++who) {
        const unsigned character = (rd16(m, kCharacterId[who]) >> 2) & 7;
        const uint8_t set = m.move_sets[character];
        if (!objs[who] || !(set & kHammer) || (set & kRayGlide) || m.glide_fall[who] <= 0) continue;
        if (who == 1 && objs[1] == objs[0]) continue;
        for (uint32_t at : {kObjHalfWidth, kObjHalfHeight}) {
            uint8_t& size = m.wram[(objs[who] + at) & 0xFFFF];
            size = uint8_t(std::min(int(size) + kHammerReach, 0x7F));
        }
    }
}

int extra_frame(const Machine& m, int who, int bank) {
    if (rd16(m, patches::kGameMode) != kModeLevel) return -1;
    const unsigned character = (rd16(m, kCharacterId[who]) >> 2) & 7;
    if (int(character) != bank || !(m.move_sets[character] & kHammer) || (m.move_sets[character] & kRayGlide)) return -1;
    const int32_t left = m.glide_fall[who];
    if (left <= 0 || left > kHammerFrames) return -1;
    return kHammerFirst + ((kHammerFrames - left) / kHammerHold) % kHammerPoses;
}

uint8_t by_name(const char* name) {
    const std::string n = name;
    if (n == "stomp") return kStomp;
    if (n == "drop_dash") return kDropDash;
    if (n == "ray_glide") return kRayGlide;
    if (n == "hammer") return kHammer;
    return 0;
}

bool any(const Machine& m) {
    for (uint8_t set : m.move_sets)
        if (set) return true;
    return false;
}

void on_chars_update(Machine& m) {
    if (!any(m) || rd16(m, patches::kGameMode) != kModeLevel) {
        m.move_state[0] = m.move_state[1] = 0;
        return;
    }
    const uint32_t objs[2] = {rd16(m, tether::kPlayerObj), rd16(m, tether::kPartnerObj)};
    const uint32_t pads[2] = {tether::kPlayerPad, tether::kPartnerPad};
    for (int who = 0; who < 2; ++who) {
        const unsigned character = (rd16(m, kCharacterId[who]) >> 2) & 7;
        const uint8_t set = m.move_sets[character];
        // The computer's partner copies the player's buttons late: it would
        // stomp after the player and never where it helps.
        const bool computer = who == 1 && tether::cpu_partner(m);
        if (!objs[who] || !set || computer || (who == 1 && objs[1] == objs[0])) {
            m.move_state[who] = 0;
            if (set & kHammer) m.glide_fall[who] = 0;
            continue;
        }
        if (set & kRayGlide) { ray_glide(m, who, objs[who], pads[who]); continue; }
        if (set & kStomp) stomp(m, who, objs[who], pads[who]);
        else m.move_state[who] &= uint8_t(~kStomping);
        if (set & kDropDash) drop_dash(m, who, objs[who], pads[who]);
        else m.move_state[who] &= uint8_t(~(kDashReleased | kDashCount));
        if (set & kHammer) hammer(m, who, objs[who], pads[who]);
    }
}

} // namespace chaotix::moves
