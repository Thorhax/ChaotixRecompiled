# Knuckles' Chaotix Recompiled — Nintendo Switch Port

This folder contains platform assets and documentation for the Nintendo Switch homebrew port of **Knuckles' Chaotix Recompiled**.

---

## 🎮 Installation & Setup

1. Copy `ChaotixRecompiled.nro` to your Switch SD card:
   ```
   sdmc:/switch/ChaotixRecompiled.nro
   ```
   or:
   ```
   sdmc:/switch/chaotix/ChaotixRecompiled.nro
   ```

2. Place your Knuckles' Chaotix 32X ROM (`chaotix.32x` or `rom.bin`) on your SD card inside:
   ```
   sdmc:/switch/chaotix/Game/chaotix.32x
   ```
   *(Alternatively, `sdmc:/switch/chaotix/` or `sdmc:/switch/chaotix/rom/`)*

3. Launch the Homebrew Menu via title override (Atmosphère), and select **Knuckles' Chaotix Recompiled**.

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

## ⚡ In-Game Cheat Codes

Hold **ZL + ZR** simultaneously and tap a direction on the **D-Pad**:

| Combination | Cheat Effect | On-Screen Toast |
| :--- | :--- | :--- |
| **Hold ZL + ZR + D-Pad Up** | Instantly gives **99 Rings** | `99 RINGS` |
| **Hold ZL + ZR + D-Pad Down** | **9 Lives / Full Recovery** (Clears ring debt, restores partner tether, and gives shields) | `PARTNER & HEALTH RESTORED (LIVES: INFINITE)` |
| **Hold ZL + ZR + D-Pad Left** | **Toggle Invincibility** powerup ON / OFF (sparkles & invulnerability) | `INVINCIBILITY: ON` / `OFF` |
| **Hold ZL + ZR + D-Pad Right** | **Enlarge (Grow)** powerup (supersizes player character) | `ENLARGE POWERUP` |

*Note: D-pad movement input to the game is automatically suppressed while holding ZL+ZR so your character does not accidentally duck or jump while triggering cheats.*

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
