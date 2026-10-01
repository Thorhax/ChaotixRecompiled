// Reference SH-2 interpreter (validation + fallback for code not covered by
// the static recompiler).
#include "sh2_interp.h"
#include "sh2_ops.h"

namespace sh2 {

namespace {

struct CachedInsn {
    uint32_t pc = 0xFFFFFFFF;
    uint16_t opcode = 0xFFFF;
    Insn in{};
};

constexpr uint32_t kCacheSize = 32768;
constexpr uint32_t kCacheMask = kCacheSize - 1;

CachedInsn g_decode_cache[2][kCacheSize];

inline const Insn& fetch_and_decode(State* c, uint32_t pc) {
    uint16_t op = uint16_t(rd16(c, pc));
    uint32_t idx = ((pc >> 1) ^ (pc >> 16)) & kCacheMask;
    CachedInsn& entry = g_decode_cache[c->id & 1][idx];
    if (__builtin_expect(entry.pc == pc && entry.opcode == op, 1)) {
        return entry.in;
    }
    entry.pc = pc;
    entry.opcode = op;
    entry.in = decode(pc, op);
    return entry.in;
}

} // namespace

void reset(State* c, uint32_t pc, uint32_t sp, uint32_t vbr) {
    Bus bus = c->bus;
    auto ack = c->irq_ack;
    auto user = c->irq_user;
    uint8_t id = c->id;
    *c = State{};
    c->bus = bus;
    c->irq_ack = ack;
    c->irq_user = user;
    c->id = id;
    c->pc = pc;
    c->r[15] = sp;
    c->vbr = vbr;
    c->sr = SR_MASK & 0xF0;  // interrupts masked
    for (CachedInsn& e : g_decode_cache[id & 1]) e = CachedInsn{};
}

void exec_branch(State* c, const Insn& in) {
    uint32_t target = branch_target(c, in);
    c->cycles += branch_cycle_adjust(in, target);
    if (in.flags & IF_DELAYED) {
        uint32_t spc = in.pc + 2;
        const Insn& slot = fetch_and_decode(c, spc);
        c->cycles -= slot.cycles;
        exec_simple(c, slot);
    }
    c->pc = target;
}

void interp_block(State* c) {
    for (;;) {
        uint32_t pc = c->pc;
        const Insn& in = fetch_and_decode(c, pc);
        c->cycles -= in.cycles;
        if (in.flags & IF_BRANCH) {
            exec_branch(c, in);
            return;
        }
        c->pc = pc + 2;
        exec_simple(c, in);
        if (ends_block(in)) return;
    }
}

} // namespace sh2
