# Knuckles' Chaotix Recompiled — Nintendo Switch Port

This folder contains platform assets and documentation for the Nintendo Switch homebrew port of **Knuckles' Chaotix Recompiled**.

---

## 🎮 Installation & Playing

1. Copy `ChaotixRecompiled.nro` (found in `dist/` or `build_switch/`) to your Switch SD card:
   ```
   sdmc:/switch/ChaotixRecompiled.nro
   ```
   or:
   ```
   sdmc:/switch/chaotix/ChaotixRecompiled.nro
   ```
2. Launch the Homebrew Menu via the Album applet or title override (Atmosphère), and select **Knuckles' Chaotix Recompiled**.

### ROM Handling
- **Embedded RomFS:** The default `.nro` binary is built with the verified ROM embedded inside `romfs:/rom.bin`, enabling instant, standalone plug-and-play without extra setup.
- **Custom / SD Card ROMs:** You can also place `rom.bin` (or any compatible 32X Chaotix ROM) in `sdmc:/switch/chaotix/`. If present, the port scans and loads from the SD card.

---

## 🕹️ Controls (Switch Default Mapping)

| Switch Button | Mega Drive / 32X Action | Notes |
| :--- | :--- | :--- |
| **D-Pad / Left Stick** | D-Pad (Up, Down, Left, Right) | Movement / Steering |
| **B (South)** | Button A | Jump / Action |
| **A (East)** | Button B | Jump / Action |
| **Y (West)** | Button C | Jump / Action |
| **X (North)** | Button X | 6-Button Pad |
| **L** | Button Y | 6-Button Pad |
| **R** | Button Z | 6-Button Pad |
| **Plus (+)** | Start | Pause / Start Game |
| **Minus (-)** | Mode / Menu | Opens in-game settings & achievements |
| **Touchscreen** | Virtual On-Screen Controls | Enabled in handheld mode |

---

## 🛠️ Building from Source

All builds use the standard devkitPro AArch64 environment (`devkitpro-mesa-rust` or `switch-dev` with `libnx`, `switch-sdl2`, and `switch-sdl2_ttf`).

### 1. Static Recompilation (Host)
Generate native C++ sources from the original ROM:
```bash
cmake -S . -B build_host -G Ninja -DCHAOTIX_BUILD_FRONTEND=OFF
cmake --build build_host
./build_host/chaotix_recomp --rom "rom/Knuckles Chaotix 32X (5) [!].bin" --out generated/
```

### 2. Switch Compilation & Packaging (.nro)
```bash
cmake -S . -B build_switch -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCHAOTIX_BUILD_TOOLS=OFF \
  -DCHAOTIX_BUILD_TESTS=OFF

cmake --build build_switch
```
The output `.nro` will be generated at `build_switch/ChaotixRecompiled.nro`.
