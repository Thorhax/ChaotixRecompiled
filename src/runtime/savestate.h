#pragma once

// Save states: the whole emulated machine at a frame boundary, as bytes.
//
// What is kept is the emulated hardware (memories, chip registers, CPU
// registers, timing) and the widescreen patch state that the game's video
// memory depends on. Host-side things are not: the ROM (a state only loads
// against the ROM it was made with, checked by SHA-1), the recompiled-code
// tables, the frontend's settings, pending audio output.

#include <cstdint>
#include <string>
#include <vector>

namespace chaotix {

class Machine;

namespace savestate {

std::vector<uint8_t> save(const Machine& m);
// Replaces the machine's state. On failure the machine is left untouched and
// `error` says why (wrong ROM, another version's format, a damaged file).
bool load(Machine& m, const std::vector<uint8_t>& data, std::string* error);

bool save_file(const Machine& m, const std::string& path, std::string* error);
bool load_file(Machine& m, const std::string& path, std::string* error);

} // namespace savestate
} // namespace chaotix
