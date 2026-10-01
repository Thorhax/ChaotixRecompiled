# Mix balance, PSG noise and the sound test (0.3.6)

Reported on the Switch port: the music's pitch and tone sounded off against a
Model 1 Genesis with a 32X, some instruments in Botanic Base stood out, and
the sound test did not respond to A, B or C. PicoDrive was the reference the
reporter found correct.

## Method

PicoDrive's libretro core was driven by a small headless host at the native
rate (53,267 Hz, YM2612 mode, no low-pass). The same scenes were rendered by
`chaotix_headless`: the boot, SEGA logo and title, and Botanic Base entered
through the game's own stage-select variables. The sources were also captured
one at a time (FM, PSG and PWM each with the other two muted). Comparisons fit
one gain, align by envelope, and report pitch and per-band level. Recordings
from RetroArch (PicoDrive) on the reporter's Switch were compared the same way.
None of these tools or recordings are part of the repository.

## Changes

- **Mix levels.** FM, PSG and PWM now use PicoDrive's relative levels and its
  overall level (including its final 7/8). Before, the PSG sat about 7.5 dB too
  high against FM, PWM about 3.5 dB too low, and the whole mix about 7 dB below
  PicoDrive's.
- **YM2612 DAC step.** The Model 1's discrete YM2612 has a step at the zero
  crossing (the "ladder effect"); negative outputs are lowered by 7 DAC steps,
  the same size as PicoDrive's YM2612 mode.
- **PSG period 0.** The Sega PSG treats a tone period of 0 as 1, where this
  implementation used 0x400. Botanic Base plays white noise clocked from tone
  2 set to 0 for almost the whole stage, so a hiss came out as a rumble.
- **PSG noise swing.** The noise channel now swings as far as a tone does (on
  the chip every channel switches between 0 and its volume); it was half.
- **Z80 bus timing.** The 68K's bus request and reset writes now first run the
  Z80 up to that moment. Before, a request made early in a slice also took the
  time before it. In the sound test the 68K holds the bus across the line on
  which the Z80's V-blank interrupt is raised, so the sound driver, which runs
  only from that interrupt, never saw the track the 68K queued on A, B or C.

## Results

| Comparison with PicoDrive | Before | After |
| --- | ---: | ---: |
| Intro: level of FM, PSG and PWM relative to each other | PSG +7.5 dB, PWM −3.5 dB | within 0.1 dB |
| Botanic Base, PSG, 8–16 kHz (level-normalised; before = before the PSG fixes) | −9.4 dB | −0.4 dB |
| Botanic Base, whole mix, any band 20 Hz–26 kHz (before = before the PSG fixes) | up to 2 dB | within 0.2 dB |
| Overall level | −7.3 dB | +0.2 dB (against the RetroArch capture) |
| Sound test, A/B/C | silent | plays the selected track |

Pitch was already identical to PicoDrive's. RetroArch on the Switch plays the
game about 14 cents lower than both, because it slows emulation to the display
rate. The real-clock pitch is kept.

Regression tests: `tests/test_psg.cpp` (period 0, noise swing),
`tests/test_z80_bus.cpp` (bus request catch-up) and the updated
`tests/test_pwm_audio.cpp` (PWM gain).
