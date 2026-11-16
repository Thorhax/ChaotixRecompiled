#include "audio/sn76489.h"
#include "test_framework.h"

#include <algorithm>
#include <vector>

using chaotix::Sn76489;

namespace {
// Noise at full volume, clocked from tone 2; the tones themselves are silent.
std::vector<int16_t> noise_from_tone2(uint8_t period_lo, uint8_t period_hi) {
    Sn76489 psg;
    psg.reset();
    for (uint8_t v : {uint8_t(0x9F), uint8_t(0xBF), uint8_t(0xDF)}) psg.write(v);
    psg.write(uint8_t(0xC0 | (period_lo & 15)));
    psg.write(period_hi);
    psg.write(0xE7);  // white noise, rate from tone 2 (resets the shifter)
    psg.write(0xF0);
    std::vector<int16_t> out;
    for (int i = 0; i < 4000; ++i) out.push_back(psg.run(16));
    return out;
}

int changes(const std::vector<int16_t>& v) {
    int n = 0;
    for (size_t i = 1; i < v.size(); ++i) n += v[i] != v[i - 1];
    return n;
}
}

TEST(psg, a_zero_period_counts_as_one) {
    // Chaotix drives its noise from tone 2 with period 0. On the Sega PSG that
    // is the fastest rate (a hiss), not the slowest (a rumble).
    const auto zero = noise_from_tone2(0, 0);
    CHECK(zero == noise_from_tone2(1, 0));
    CHECK(changes(zero) > 50 * changes(noise_from_tone2(15, 0x3F)));
}

TEST(psg, noise_swings_as_far_as_a_tone) {
    const auto noise = noise_from_tone2(1, 0);
    const auto [lo, hi] = std::minmax_element(noise.begin(), noise.end());
    CHECK(*hi > 0);
    CHECK_EQ(int(*lo), -int(*hi));
}
