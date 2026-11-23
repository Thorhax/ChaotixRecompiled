# Knuckles' Chaotix Recompiled on the Nintendo Switch

A homebrew `.nro` of [Knuckles' Chaotix Recompiled](https://github.com/YuutaTsubasa/ChaotixRecompiled)
by YuutaTsubasa. The recompiler, the runtime and the frontend are upstream's;
this fork adds what the Switch needs and follows upstream's releases. Since
1.0.1 the port has a version of its own (`v1.0.1`, shown as `1.0.1` in the
homebrew menu); each release's notes say which upstream version it is made
from (1.0.1: upstream 0.15.23). Builds before it were numbered after upstream:
`0.15.22-switch.1` was the first Switch build of upstream 0.15.22.

It needs a console that runs homebrew (Atmosphère) and **your own** ROM of
*Knuckles' Chaotix (Japan, USA)* for the 32X, SHA-1
`0c2fff7bc79ed26507c08ac47464c3af19f7ced7`. The `.nro` contains no game data
and will not start the game without it. See [LEGAL.md](../../LEGAL.md).

## Installing

1. Copy `ChaotixRecompiled.nro` to `sdmc:/switch/chaotix/` (or anywhere under
   `sdmc:/switch/`).
2. Copy your ROM to `sdmc:/switch/chaotix/`. Any name ending in `.32x`,
   `.bin` or `.md` will do.
3. Start it from the homebrew menu. Open the homebrew menu through a game
   (hold R while starting one) rather than the album, so it has the memory
   of an application.
4. The first start shows the ROM it found, marked VERIFIED. Press **A** to
   install it: it is copied to `sdmc:/switch/chaotix/Game/chaotix.32x`, and
   the copy you made in step 2 can then be deleted.

If no ROM is found, the page says so and looks again every couple of seconds,
so the ROM can be copied over (FTP, MTP) while it is up. **Y** looks again at
once, **+** leaves. Besides its own folder it looks in `sdmc:/`,
`sdmc:/switch/` and `sdmc:/roms/` (`32x`, `sega32x`, `megadrive`).

Everything the game keeps is under `sdmc:/switch/chaotix/`: `Config/`,
`SaveData/` (the cartridge's save and the awards), `States/`, `Mods/` and
`Cache/`.

### Coming from the 1.0.0 Switch release

The same folder is used, and the cartridge save and the awards carry over.
Three things do not:

- **Save states** of 1.0.0 and earlier are in another format and are not
  listed. They are left on the card (`States/slot1.state` ...).
- **ZL + ZR + D-pad cheats** are gone: ZL and ZR are rewind and fast-forward
  now. Cheats are under OPTIONS > EXTRAS > CHEATS (99 RINGS, SPEED SHOES, and
  Action Replay codes in `Config/chaotix.ini`; see [docs/CHEATS.md](../../docs/CHEATS.md)).
  Awards are not earned while one is on.
- **A confirms and B goes back** in the menus, as on the console itself.
  1.0.0 had them the other way round.

## Controls

| Joy-Con | In the game | In the menus |
| :--- | :--- | :--- |
| D-pad / left stick | Move | Choose |
| **Y** | A: pull the partner in | |
| **B** | B: hold the partner | Back |
| **A** | C: jump | Select |
| **L**, **X**, **R** | X, Y, Z (the game does not use them) | |
| **+** | Start: pause | |
| **−** | Opens the menu | Back |
| **ZR** (hold) | Fast-forward | |
| **ZL** (hold) | Rewind, up to the last minute | |

Every one of them can be changed under CONTROLS. A second player is
CONTROLS > 2P DEVICE > CONTROLLER 2.

The menu (**−**) has save states (STATES: four slots), the options, the awards,
TIME ATTACK and LEVEL SELECT from the title, and the rest of what upstream's
[README](../../README.md) describes. What a console has no use for is left
out: keyboard bindings, the window mode, the touch controls and the update
check, which asks about upstream's releases.

The game goes on under the menu, as it does upstream; **+** is the game's own
pause.

## Building

The Switch has SDL 2 and no SDL 3, which the frontend is written for.
`platforms/switch/sdl3/` stands in for it: headers named like SDL 3's that
give the calls the frontend uses in terms of SDL 2, put ahead of everything
else on the include path. The frontend itself is upstream's, with the
console's differences under `#ifdef __SWITCH__`.

Two steps, as for the other cross builds: the recompiler runs on the PC, and
the Switch build compiles what it wrote.

```bash
# 1. On the PC: recompile your ROM into generated/ (and run the tests).
cmake -S . -B build_host -G Ninja -DCMAKE_BUILD_TYPE=Release -DCHAOTIX_ROM=/path/to/chaotix.32x
cmake --build build_host
ctest --test-dir build_host --output-on-failure

# 2. The .nro, in devkitPro's image (or with devkitA64, switch-sdl2 and
#    switch-sdl2_ttf installed: cmake --preset switch).
docker run --rm -u "$(id -u):$(id -g)" -v "$PWD":/work -w /work devkitpro/devkita64 bash -c \
  'cmake -S . -B build_switch -G Ninja -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake \
         -DCMAKE_BUILD_TYPE=Release -DCHAOTIX_BUILD_TOOLS=OFF -DCHAOTIX_BUILD_TESTS=OFF &&
   cmake --build build_switch'
```

The result is `build_switch/ChaotixRecompiled.nro`. It carries no RomFS, so
no file of yours ends up inside it; `generated/` is compiled into it, which is
what [LEGAL.md](../../LEGAL.md) is about.

Started with `nxlink -s`, the program sends its log back to the PC.

## What is different from upstream

- `platforms/switch/sdl3/`: the SDL 3 names on SDL 2, and SDL 3's debug text
  drawn with the embedded pixel font.
- `platforms/switch/switch_threads.cpp`: a megabyte of stack for every thread.
  The console gives 128 KiB, and the menu's zone pictures and a time attack
  ghost run a whole machine on theirs.
- `src/frontend/setup.cpp`: copying the ROM falls back to plain streams where
  `std::filesystem::copy_file` does not work, as on the Switch.
- `src/net/udp_socket.cpp`: IPv4 only on the Switch, which has no IPv6.
- `src/audio/sn76489.cpp`: a PSG tone period of 0 counts as 1, as on Sega's
  chip. The game clocks its noise from such a tone, so this is a hiss where
  the TI part's 0x400 gives a rumble (`tests/test_psg.cpp`).
- The setup page, the menu's hints and button names, the defaults and the
  paths, under `#ifdef __SWITCH__` in `src/platform` and `src/frontend/config.cpp`.

NETPLAY is built and its page opens, but it has not been played on a console.
