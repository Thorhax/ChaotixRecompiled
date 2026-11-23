// See sound_mods.h.
#include "audio/sound_mods.h"
#include "runtime/log.h"
#include "runtime/moves.h"
#include "runtime/system.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;

namespace chaotix::audio {

namespace {

// The files' names, by moves::kSoundFly....
const char* const kNames[moves::kSoundKinds] = {nullptr,       "tails_flight",     "tails_flight_tired",
                                                 "glide_up",    "glide_down",       "drop_dash_charge",
                                                 "drop_dash_release", "hammer"};

// A file at full scale against the game's own sound effects: Sonic Mania's
// flight, played at this, is about as loud as the game's FM effects.
constexpr float kGain = 0.5f;
// Frames without an ask a sound carries on through.
constexpr int kCarryOn = 3;

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return char(std::tolower(ch)); });
    return s;
}

} // namespace

int SoundMods::scan(const std::string& dir) {
    std::error_code ec;
    for (fs::directory_iterator it(fs::u8path(dir), ec), end; !ec && it != end; it.increment(ec)) {
        const fs::path& p = it->path();
        if (lower(p.extension().u8string()) != ".wav") continue;
        const std::string stem = lower(p.stem().u8string());
        for (uint8_t s = 1; s < moves::kSoundKinds; ++s) {
            if (stem != kNames[s]) continue;
            std::ifstream f(p, std::ios::binary);
            const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            Track t;
            std::string error;
            if (bytes.empty() || !decode_wav(bytes, kAudioRate, t, &error) || !t.frames) {
                LOGW("sounds", "cannot use %s: %s", p.u8string().c_str(), error.empty() ? "cannot read it" : error.c_str());
                break;
            }
            files_[s] = std::move(t);
        }
    }
    int n = 0;
    for (const Track& t : files_) n += t.frames ? 1 : 0;
    return n;
}

void SoundMods::set_sound(uint8_t sound, Track track) {
    if (sound < files_.size()) files_[sound] = std::move(track);
}

void SoundMods::attach(Machine& m) const {
    m.move_sound_files = 0;
    for (uint8_t s = 1; s < files_.size(); ++s)
        if (files_[s].frames) m.move_sound_files |= moves::sound_bit(s);
}

void SoundMods::after_frame(Machine& m, size_t first_sample, float level) {
    // Again each frame: a machine put back (a state, the title's copy) may
    // carry another's.
    attach(m);
    const size_t total = m.audio_out.size();
    const size_t frames = m.audio_enabled && first_sample <= total ? (total - first_sample) / 2 : 0;
    // A sound played once: from its start, to its end; a later one on the
    // same character takes its place.
    for (size_t who = 0; who < shots_.size(); ++who) {
        Voice& v = shots_[who];
        const uint8_t want = m.move_shot[who];
        m.move_shot[who] = moves::kSoundNone;
        if (want >= moves::kSoundFirstShot && want < files_.size() && files_[want].frames) v = Voice{want, 0, 0};
        if (!v.sound) continue;
        const Track& t = files_[v.sound];
        for (size_t k = 0; k < frames && v.pos < t.frames; ++k, ++v.pos)
            for (int ch = 0; ch < 2; ++ch) {
                int16_t& s = m.audio_out[first_sample + k * 2 + size_t(ch)];
                const float add = float(t.samples[v.pos * 2 + size_t(ch)]) * kGain * level;
                s = int16_t(std::clamp(int(s) + int(std::lround(add)), -32768, 32767));
            }
        if (v.pos >= t.frames) v = Voice{};
    }
    for (size_t who = 0; who < voices_.size(); ++who) {
        Voice& v = voices_[who];
        uint8_t want = m.move_sound[who];
        m.move_sound[who] = moves::kSoundNone;
        // A frame the game's objects did not run in (it lags now and then)
        // asks for nothing: the sound carries on through a few of them.
        if (want != moves::kSoundNone) v.missed = 0;
        else if (v.sound && ++v.missed <= kCarryOn) want = v.sound;
        if (want >= files_.size() || !files_[want].frames) {
            v = Voice{};
            continue;
        }
        if (v.sound != want) v = Voice{want, 0, 0};
        const Track& t = files_[want];
        const size_t loop_end = t.loops && t.loop_end > t.loop_start && t.loop_end <= t.frames ? t.loop_end : t.frames;
        const size_t loop_start = loop_end == t.loop_end && t.loops ? t.loop_start : 0;
        for (size_t k = 0; k < frames; ++k) {
            if (v.pos >= loop_end) v.pos = loop_start;
            for (int ch = 0; ch < 2; ++ch) {
                int16_t& s = m.audio_out[first_sample + k * 2 + size_t(ch)];
                const float add = float(t.samples[v.pos * 2 + size_t(ch)]) * kGain * level;
                s = int16_t(std::clamp(int(s) + int(std::lround(add)), -32768, 32767));
            }
            ++v.pos;
        }
    }
}

} // namespace chaotix::audio
