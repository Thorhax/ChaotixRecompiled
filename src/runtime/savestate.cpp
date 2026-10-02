#include "runtime/savestate.h"
#include "runtime/system.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <type_traits>

namespace chaotix::savestate {

namespace {

constexpr char kMagic[8] = {'C', 'H', 'X', 'S', 'T', 'A', 'T', 'E'};
// Bump whenever anything below changes size, order or meaning.
constexpr uint32_t kVersion = 1;

struct Writer {
    std::vector<uint8_t>& out;
    void bytes(const void* p, size_t n) {
        const auto* b = static_cast<const uint8_t*>(p);
        out.insert(out.end(), b, b + n);
    }
    template <class T> void pod(const T& v) {
        static_assert(std::is_trivially_copyable_v<T>, "raw copy needs a trivially copyable type");
        bytes(&v, sizeof v);
    }
};

struct Reader {
    const std::vector<uint8_t>& in;
    size_t pos = 0;
    bool ok = true;
    void bytes(void* p, size_t n) {
        if (!ok || in.size() - pos < n) { ok = false; return; }
        std::memcpy(p, in.data() + pos, n);
        pos += n;
    }
    template <class T> void pod(T& v) {
        static_assert(std::is_trivially_copyable_v<T>, "raw copy needs a trivially copyable type");
        bytes(&v, sizeof v);
    }
};

// The CPU states end in bus wiring (pointers into this process); only the
// registers in front of it are state.
constexpr size_t kM68kRegs = offsetof(m68k::State, bus);
constexpr size_t kSh2Regs = offsetof(sh2::State, bus);
constexpr size_t kZ80Regs = offsetof(z80::State, bus);

// One list for both directions, so save and load cannot drift apart. `S` is
// Writer or Reader; `M` is const Machine or Machine accordingly.
template <class S, class M> void fields(S& s, M& m) {
    s.pod(m.wram);
    s.pod(m.sdram);
    s.pod(m.zram);
    s.pod(m.sram);
    s.pod(m.sram_ctrl);
    s.pod(m.vdp);  // its host pointers are put back by Machine::rebind
    s.pod(m.mars);
    s.pod(m.pwm.ctrl);
    s.pod(m.pwm.cycle);
    s.pod(m.pwm.fifo_l);
    s.pod(m.pwm.fifo_r);
    s.pod(m.pwm.count_l);
    s.pod(m.pwm.count_r);
    s.pod(m.pwm.timer);
    s.pod(m.pwm.accum);
    s.pod(m.pwm.out_l);
    s.pod(m.pwm.out_r);
    s.pod(m.onchip);
    s.bytes(const_cast<m68k::State*>(&m.m68k), kM68kRegs);
    for (auto& c : m.sh2) s.bytes(const_cast<sh2::State*>(&c), kSh2Regs);
    s.bytes(const_cast<z80::State*>(&m.z80), kZ80Regs);
    s.pod(m.ym);
    s.pod(m.psg);
    s.pod(m.z80_busreq);
    s.pod(m.z80_reset);
    s.pod(m.z80_bank);
    s.pod(m.audio_pos);
    s.pod(m.psg_frac);
    s.pod(m.dc_x);
    s.pod(m.dc_y);
    s.pod(m.io_ctrl);
    s.pod(m.io_data);
    s.pod(m.pad_th_count);
    s.pod(m.pad_th_time);
    s.pod(m.mclk);
    s.pod(m.m68k_clock);
    s.pod(m.sh2_clock);
    s.pod(m.z80_clock);
    s.pod(m.line);
    s.pod(m.line_start_mclk);
    s.pod(m.frame_count);
    s.pod(m.boot_hle_done);
    // Widescreen: the level engine streams tiles for the shift latched at the
    // level's load, so the state of the patch travels with the video memory.
    s.pod(m.plane_shift);
    s.pod(m.level_seen_frame);
    s.pod(m.wide_active);
    s.pod(m.clip_overridden);
    s.pod(m.cull_shift);
    s.pod(m.plane_fill_clamped);
    s.pod(m.fb_margin);
}

} // namespace

std::vector<uint8_t> save(const Machine& m) {
    std::vector<uint8_t> out;
    Writer w{out};
    w.bytes(kMagic, sizeof kMagic);
    w.pod(kVersion);
    char sha[40] = {};
    std::memcpy(sha, m.rom.sha1.data(), std::min(sizeof sha, m.rom.sha1.size()));
    w.bytes(sha, sizeof sha);
    fields(w, m);
    return out;
}

bool load(Machine& m, const std::vector<uint8_t>& data, std::string* error) {
    auto fail = [error](const char* why) {
        if (error) *error = why;
        return false;
    };
    Reader r{data};
    char magic[sizeof kMagic];
    uint32_t version = 0;
    char sha[40];
    r.bytes(magic, sizeof magic);
    r.pod(version);
    r.bytes(sha, sizeof sha);
    if (!r.ok || std::memcmp(magic, kMagic, sizeof kMagic) != 0) return fail("not a save state");
    if (version != kVersion) return fail("made by a different version");
    if (m.rom.sha1.size() != sizeof sha || std::memcmp(sha, m.rom.sha1.data(), sizeof sha) != 0)
        return fail("made with a different ROM");

    // Read into a copy first, so a short or damaged file changes nothing.
    // The copy carries everything the state leaves alone: the ROM, settings,
    // input, the recompiled-code ranges.
    auto next = std::make_unique<Machine>(m);
    fields(r, *next);
    if (!r.ok || r.pos != data.size()) return fail("damaged or incomplete");
    // The raw VDP copy carries function pointers from the process that saved
    // it; after a restart the code sits elsewhere, so keep this process's.
    next->vdp.host = m.vdp.host;
    next->vdp.on_vram_write = m.vdp.on_vram_write;
    next->vdp.on_vram_write_user = m.vdp.on_vram_write_user;

    m = *next;
    m.stage_pending = false;
    m.audio_out.clear();
    m.pwm.samples.clear();
    // Memory holding recompiled SH-2 code may differ now: make the dispatcher
    // check it again rather than trust what it saw before.
    ++m.sdram_code_epoch;
    for (auto& e : m.cache_code_epoch) ++e;
    m.rebind();
    return true;
}

bool save_file(const Machine& m, const std::string& path, std::string* error) {
    const std::vector<uint8_t> data = save(m);
    const std::string tmp = path + ".tmp";
    FILE* f = std::fopen(tmp.c_str(), "wb");
    if (!f) { if (error) *error = "cannot write " + tmp; return false; }
    const bool ok = std::fwrite(data.data(), 1, data.size(), f) == data.size();
    if (std::fclose(f) != 0 || !ok) { std::remove(tmp.c_str()); if (error) *error = "write failed"; return false; }
    // Replace the old state only once the new one is complete.
    std::remove(path.c_str());
    if (std::rename(tmp.c_str(), path.c_str()) != 0) { if (error) *error = "cannot replace " + path; return false; }
    return true;
}

bool load_file(Machine& m, const std::string& path, std::string* error) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) { if (error) *error = "no state in this slot"; return false; }
    std::vector<uint8_t> data;
    uint8_t buf[65536];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) data.insert(data.end(), buf, buf + n);
    std::fclose(f);
    return load(m, data, error);
}

} // namespace chaotix::savestate
