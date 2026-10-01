#include "runtime/system.h"
#include "test_framework.h"

#include <cstring>
#include <memory>

using namespace chaotix;

TEST(z80_bus, a_bus_request_lets_the_z80_run_up_to_that_moment) {
    auto m = std::make_unique<Machine>();
    m->rom.data.resize(0x400000);
    m->reset();
    // ld hl,$1000 / loop: inc (hl) / jr loop
    const uint8_t program[] = {0x21, 0x00, 0x10, 0x34, 0x18, 0xFD};
    std::memcpy(m->zram, program, sizeof program);
    m->zram[0x1000] = 0;
    m->set_z80_reset(false);

    // The 68K takes the bus part-way into a slice: the Z80 must have run until
    // then, or a one-line interrupt in that window is lost (the sound test).
    m->m68k_clock = 3000;
    m->set_z80_busreq(true);
    CHECK_EQ(m->z80_clock, uint64_t(3000 * 7 / 15));
    const uint8_t ran = m->zram[0x1000];
    CHECK(ran > 0);

    // Off the bus, time passes without the Z80 running.
    m->z80_catch_up(42000);
    CHECK_EQ(int(m->zram[0x1000]), int(ran));
    CHECK_EQ(m->z80_clock, uint64_t(42000 / 15));
}
