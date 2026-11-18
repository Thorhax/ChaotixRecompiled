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
        if (state == kStateGround || state == kStateRoll) m.wram[(pad + 3) & 0xFFFF] |= kJump;
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
    const int had = int16_t(rd16(m, obj + kObjGroundSpeed));
    const int along = left ? -had : had;
    int speed = kDropDashSpeed;
    if (along > 0) speed = std::min(kDropDashSpeed + along / 4, kDropDashMax);
    const int v = left ? -speed : speed;
    LOGD("moves", "drop dash: character %d, ground speed %d", who, v);
    wr16(m, obj + kObjGroundSpeed, uint16_t(v));
    wr32(m, obj + kObjXVel, uint32_t(int32_t(v) * 256));
    uint8_t& flags = m.wram[(obj + kObjFlags) & 0xFFFF];
    flags = uint8_t(left ? flags | 0x08 : flags & ~0x08);
    m.wram[(obj + kObjState) & 0xFFFF] = kStateRoll;
    wr16(m, obj + kObjAnim, kAnimBall);
    post_sound(m, kSoundDashRelease);
}

constexpr uint16_t kAnimGlide = 0x40;
constexpr uint8_t kStateGlide = 0x24;
constexpr uint32_t kObjY = 0x0C;          // long, 16.16
constexpr uint32_t kObjAnimFrame = 0x28;  // 8.8

void ray_glide(Machine& m, int who, uint32_t obj, uint32_t pad) {
    uint8_t& now = m.move_state[who];
    int32_t& fall = m.glide_fall[who];
    uint8_t& state = m.wram[(obj + kObjState) & 0xFFFF];
    uint8_t& held = m.wram[(pad + 2) & 0xFFFF];
    uint8_t& flags = m.wram[(obj + kObjFlags) & 0xFFFF];
    if (!(now & kGlideOn)) {
        now = 0;
        uint8_t& pressed = m.wram[(pad + 3) & 0xFFFF];
        const bool in_air = state == kStateJump || state == kStateFall;
        if (!in_air || rd16(m, obj + kObjAnim) != kAnimBall || !(pressed & kJump)) return;
        pressed &= uint8_t(~kJump);
        const bool left = (flags & 0x08) != 0;
        now = uint8_t(kGlideOn | (left ? kGlideLeft : 0));
        const int32_t vx = int32_t(rd32(m, obj + kObjXVel));
        const int32_t speed = std::max(left ? -vx : vx, kGlideStart);
        fall = int32_t(rd32(m, obj + kObjYVel));
        if (held & (left ? 0x08 : 0x04)) {  // back held: up at once
            now |= kGlideUp;
            fall = std::min(fall, -(speed / 2) - 0x10000);
        }
        wr32(m, obj + kObjXVel, uint32_t(left ? -speed : speed));
        state = kStateGlide;
        wr16(m, obj + kObjAnim, kAnimGlide);
        wr16(m, obj + kObjAnimFrame, 0);
        LOGD("moves", "glide: character %d", who);
    } else if (state != kStateGlide) {
        // The ground, a hit, a wall: over. That state's code takes hold of a
        // wall as Knuckles does ($26, $28): Ray does not climb, he drops.
        now = 0;
        if (state == 0x26 || state == 0x28) {
            state = kStateFall;
            wr32(m, obj + kObjXVel, 0);
            wr32(m, obj + kObjYVel, 0);
        }
        return;
    } else {
        const bool left = (now & kGlideLeft) != 0;
        const int32_t vx = int32_t(rd32(m, obj + kObjXVel));
        int32_t speed = left ? -vx : vx;
        if (speed < kGlideMinSpeed / 2) {
            // Stopped dead by something in the way: he drops.
            now = 0;
            state = kStateFall;
            wr32(m, obj + kObjYVel, uint32_t(std::max(fall, 0)));
            return;
        }
        const bool back = (held & (left ? 0x08 : 0x04)) != 0, forward = (held & (left ? 0x04 : 0x08)) != 0;
        if (now & kGlideUp) {
            if (forward) {
                now &= uint8_t(~kGlideUp);
            } else {
                fall = std::min(fall + kGlideClimbFall, kGlideSink);
                if (fall < 0) speed = std::max(speed - kGlideClimbDrag, kGlideMinSpeed);
            }
        }
        if (!(now & kGlideUp)) {
            if (back && speed > kGlideMinSpeed + 0x8000) {
                // Pull up: the speed he has, and half the fall, become height.
                now |= kGlideUp;
                fall = -std::clamp(speed * 3 / 4 + std::max(fall, 0) / 2, 0x20000, 0x90000);
            } else {
                fall = std::min(fall + kGlideDiveFall, kGlideMaxFall);
                speed = std::min(speed + kGlideDivePush, kGlideMaxSpeed);
            }
        }
        wr32(m, obj + kObjXVel, uint32_t(left ? -speed : speed));
        flags = uint8_t(left ? flags | 0x08 : flags & ~0x08);
    }
    // The state's code moves him by the X velocity and by its own small fall:
    // the rest of the fall is added to where he is.
    wr32(m, obj + kObjY, rd32(m, obj + kObjY) + uint32_t(fall - kGlideOwnFall));
    // What that code reads: jump held (or it drops him), no direction (or it turns him).
    held = uint8_t((held | kJump) & ~kLeftRight);
    wr16(m, pad + 8, 0);
}

} // namespace

uint8_t by_name(const char* name) {
    const std::string n = name;
    if (n == "stomp") return kStomp;
    if (n == "drop_dash") return kDropDash;
    if (n == "ray_glide") return kRayGlide;
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
            continue;
        }
        if (set & kRayGlide) { ray_glide(m, who, objs[who], pads[who]); continue; }
        if (set & kStomp) stomp(m, who, objs[who], pads[who]);
        else m.move_state[who] &= uint8_t(~kStomping);
        if (set & kDropDash) drop_dash(m, who, objs[who], pads[who]);
        else m.move_state[who] &= uint8_t(~(kDashReleased | kDashCount));
    }
}

} // namespace chaotix::moves
