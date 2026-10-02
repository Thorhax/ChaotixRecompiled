#include "runtime/savestate.h"
#include "runtime/system.h"
#include "test_framework.h"

#include <cstring>
#include <memory>

using namespace chaotix;

namespace {
// A machine with a synthetic cartridge, halted 68K and a Z80 counting in RAM,
// so frames change state without a ROM.
std::unique_ptr<Machine> counting_machine() {
    auto m = std::make_unique<Machine>();
    m->rom.data.resize(0x400000);
    m->rom.sha1 = std::string(40, 'a');
    m->reset();
    m->m68k.stopped = true;
    m->audio_enabled = true;
    // ld hl,$1000 / loop: inc (hl) / jr loop
    const uint8_t program[] = {0x21, 0x00, 0x10, 0x34, 0x18, 0xFD};
    std::memcpy(m->zram, program, sizeof program);
    m->set_z80_reset(false);
    m->wram[0x1234] = 0x56;
    return m;
}
}

TEST(savestate, a_loaded_state_runs_on_like_the_original) {
    auto a = counting_machine();
    a->run_frame();
    const auto state = savestate::save(*a);

    auto b = std::make_unique<Machine>();
    b->rom.data.resize(0x400000);
    b->rom.sha1 = a->rom.sha1;
    b->reset();
    b->audio_enabled = true;  // a host setting: the state leaves it alone
    std::string why;
    CHECK(savestate::load(*b, state, &why));
    CHECK_EQ(int(b->wram[0x1234]), 0x56);

    a->audio_out.clear();
    a->run_frame();
    b->run_frame();
    CHECK(std::memcmp(a->zram, b->zram, sizeof a->zram) == 0);
    CHECK_EQ(a->z80.pc, b->z80.pc);
    CHECK_EQ(a->frame_count, b->frame_count);
    CHECK(a->audio_out == b->audio_out);
    // The loaded machine's buses lead to itself, not to the one it came from.
    CHECK(b->z80.bus.user == b.get());
    CHECK(b->vdp.host.user == b.get());
}

TEST(savestate, refuses_another_rom_and_damaged_data) {
    auto a = counting_machine();
    auto state = savestate::save(*a);
    auto b = counting_machine();
    b->wram[0x1234] = 0x99;

    std::string why;
    b->rom.sha1 = std::string(40, 'b');
    CHECK(!savestate::load(*b, state, &why));
    b->rom.sha1 = a->rom.sha1;

    auto cut = state;
    cut.resize(cut.size() - 1);
    CHECK(!savestate::load(*b, cut, &why));
    CHECK(!savestate::load(*b, {}, &why));
    // Nothing was applied by the failed loads.
    CHECK_EQ(int(b->wram[0x1234]), 0x99);
}

TEST(savestate, a_state_from_another_run_keeps_this_runs_code_pointers) {
    // A state written by an earlier run holds that run's code addresses,
    // which mean nothing once the program is loaded somewhere else.
    auto a = counting_machine();
    a->vdp.host.dma_read16 = reinterpret_cast<uint16_t (*)(void*, uint32_t)>(uintptr_t(0xDEAD0000));
    a->vdp.on_vram_write = reinterpret_cast<void (*)(void*, uint32_t, uint16_t)>(uintptr_t(0xDEAD0010));
    const auto state = savestate::save(*a);

    auto b = counting_machine();
    const auto dma = b->vdp.host.dma_read16;
    const auto vram_hook = b->vdp.on_vram_write;
    std::string why;
    CHECK(savestate::load(*b, state, &why));
    CHECK(b->vdp.host.dma_read16 == dma);
    CHECK(b->vdp.on_vram_write == vram_hook);
}
