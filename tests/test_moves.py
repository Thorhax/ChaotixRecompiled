#!/usr/bin/env python3
"""The extra moves (runtime/moves.h), measured in the game.

Mighty in Botanic Base 1: a jump, and jump pressed again in the air.
  - with the stomp: he stops dead (X velocity 0), falls at the stomp's speed,
    lands sooner than the jump alone would, and hops (a short jump);
  - with left or right held as well (on the move, as Sonic Mania's drop),
    a few frames later: the game has the press first, to cling to a wall;
    without the move the second press does nothing of the kind.
  - the wall cling (state $20) is the same with the stomp on as without it;
  - spikes (object $54 made under him) hurt a plain landing, but the stomp
    lands on them unhurt and rebounds a full jump high.
Usage: test_moves.py <chaotix_headless> <rom>
"""
import subprocess
import sys

HEADLESS, ROM = sys.argv[1], sys.argv[2]
PLAYER = 0xAD48  # the player's object in Botanic Base 1
STOMP_SPEED = 0x00100000
WINDUP = 4  # moves::kStompWindup
FAILED = []


def check(ok, what):
    print(("ok   " if ok else "FAIL ") + what)
    if not ok:
        FAILED.append(what)


def s32(v):
    return v - (1 << 32) if v & 0x80000000 else v


def run(*extra):
    """Per frame: (state, x velocity, y velocity, ground speed, x, facing left) of the player's object.

    Ground speed counts forward, the way the character faces."""
    out = subprocess.run(
        [HEADLESS, "--rom", ROM, "--frames", "960", "--stage", "300:0:1:0:0:7:0",
         "--peek-every", "1:%04X:48" % PLAYER, *extra],
        capture_output=True, text=True, check=True).stdout
    frames = {}
    for line in out.splitlines():
        p = line.split()
        if len(p) == 4 and p[0] == "peek":
            b = bytes.fromhex(p[3])
            frames[int(p[1])] = (b[7], s32(int.from_bytes(b[0x18:0x1C], "big")),
                                 s32(int.from_bytes(b[0x1C:0x20], "big")),
                                 int.from_bytes(b[0x2C:0x2E], "big", signed=True),
                                 int.from_bytes(b[8:10], "big"), b[0x25] & 8 != 0)
    return frames


JUMP = ["--press", "850:c:12"]
AGAIN = ["--press", "866:c:3"]


def landing(frames, after):
    return next(f for f in sorted(frames) if f > after and frames[f][0] == 0x02)


plain = run(*JUMP)
check(plain[860][0] == 0x04, "a jump is state 04")
plain_landing = landing(plain, 855)

stomp = run("--moves", "0:1", *JUMP, *AGAIN)
# (Peeks are after the frame: the game's gravity, under 1/4 px, is on top of the hook's 0.)
check(all(stomp[f][1] == 0 and 0 <= stomp[f][2] < 0x4000 for f in range(867, 867 + WINDUP)),
      "jump again in the air: he stops dead for the wind-up (%d frames)" % WINDUP)
check(stomp[867 + WINDUP][0] == 0x08 and stomp[867 + WINDUP][1] == 0 and stomp[867 + WINDUP][2] >= STOMP_SPEED,
      "then straight down at the stomp's speed (y velocity %#x)" % stomp[867 + WINDUP][2])
stomp_landing = landing(stomp, 866)
check(stomp_landing < plain_landing - 10,
      "the stomp lands sooner (frame %d, the jump alone %d)" % (stomp_landing, plain_landing))
hop = stomp[stomp_landing + 2]
check(hop[0] == 0x04 and -0x40000 <= hop[2] < 0,
      "and hops on landing (state %02X, y velocity %#x)" % (hop[0], hop[2] & 0xFFFFFFFF))
hop_landing = landing(stomp, stomp_landing + 2)
check(all(stomp[f][2] < STOMP_SPEED for f in range(stomp_landing + 1, hop_landing)),
      "the hop is a hop, not another stomp")

off = run(*JUMP, *AGAIN)
check(all(off[f] == plain[f] for f in range(840, 960)), "without the move the second press changes nothing")

held = run("--moves", "0:1", "--press", "850:c:12", "--press", "850:right:30", "--press", "866:c+right:3")
# (The game adds its air control, 1/16 px a frame, after the hook clears X.)
# Jump let go at 869 starts it; then the wind-up.
DROP = 869 + WINDUP + 1
check(held[868][1] != 0 and abs(held[DROP][1]) <= 0x1000 and held[DROP][2] >= STOMP_SPEED,
      "moving right with right held: no wall, so the stomp follows (x velocity %#x, y velocity %#x)"
      % (held[DROP][1] & 0xFFFFFFFF, held[DROP][2] & 0xFFFFFFFF))

# The wall cling: run left into the wall at x 141, jump, press jump again by
# the wall. The game clings (state $20) at the same presses with the stomp on.
for again in (916, 920, 924):
    presses = ("--press", "850:left:150", "--press", "880:c:6", "--press", "%d:c+left:30" % again)
    off_cling = any(v[0] == 0x20 for v in run(*presses).values())
    on_cling = any(v[0] == 0x20 for v in run("--moves", "0:1", *presses).values())
    check(off_cling and on_cling, "jump again at the wall (frame %d): he clings, the stomp on (%s) or off (%s)"
          % (again, on_cling, off_cling))

# Spikes: object $FF8040 made spikes ($54, pointing up) under him in his jump.
SPIKES = []
for addr, v in ((0x8046, 0), (0x8047, 0x54), (0x8068, 0), (0x8048, 264 >> 8), (0x8049, 264 & 255),
                (0x804C, 1647 >> 8), (0x804D, 1647 & 255)):
    SPIKES += ["--poke", "856:%04X:%02X" % (addr, v)]
plain_spikes = run(*JUMP, *SPIKES)
check(any(plain_spikes[f][0] == 0x12 for f in range(880, 930)), "a plain landing on the spikes hurts (state 12)")
drop = run("--moves", "0:1", *JUMP, *SPIKES, "--press", "878:c:3")
hit = next(f for f in range(879, 900) if drop[f][2] < 0)
check(all(drop[f][0] != 0x12 for f in range(878, hit + 30)), "the stomp onto the spikes does not hurt")
check(drop[hit][2] <= -0x60000, "and rebounds a full jump high (y velocity %#x)" % (drop[hit][2] & 0xFFFFFFFF))


# The drop dash: jump, let go, hold jump again until landing.
DASH_SPEED = 0x0C00  # ground speed, 8.8
dash = run("--moves", "0:2", *JUMP, "--press", "866:c:80")
dash_landing = next(f for f in sorted(dash) if f > 866 and dash[f][0] in (0x02, 0x06))
after = dash[dash_landing + 2]
check(after[0] == 0x06 and DASH_SPEED - 0x40 <= after[3] <= DASH_SPEED,
      "drop dash: lands rolling at the dash's speed (state %02X, ground speed %#x)" % (after[0], after[3]))
check(all(dash[f] == plain[f] for f in range(840, dash_landing - 1)), "and the jump itself is unchanged")
short = run("--moves", "0:2", *JUMP, "--press", "866:c:10")
check(all(short[f] == plain[f] for f in range(840, 960)), "let go before landing: no dash")
late = run("--moves", "0:2", "--press", "850:c:80")
check(all(late[f][3] == 0 for f in range(850, 940)), "jump held all the way from the ground: no dash")
left = run("--moves", "0:2", *JUMP, "--press", "866:c:80", "--press", "880:left:60")
left_landing = next(f for f in sorted(left) if f > 866 and left[f][0] in (0x02, 0x06))
after = left[left_landing + 4]  # (a wall stops it a few frames later)
check(after[0] == 0x06 and after[5] and after[3] >= DASH_SPEED - 0x100 and after[4] < left[left_landing][4] - 30,
      "left held: the dash goes left (ground speed %d, x %d to %d)" % (after[3], left[left_landing][4], after[4]))
# Facing left with nothing held (the partner behind, to the right): the dash
# still goes left (it went right when ground speed was taken for left-to-right).
away = run("--moves", "0:2", "--frames", "1200", "--press", "860:right:200", "--press", "1062:left:60",
           "--press", "1124:c:6", "--press", "1134:c:50")
away_landing = next(f for f in sorted(away) if f > 1134 and away[f][0] in (0x02, 0x06))
after = away[away_landing + 8]
check(after[0] == 0x06 and after[5] and after[4] < away[away_landing][4] - 40,
      "facing left, nothing held: the dash goes left (x %d to %d)" % (away[away_landing][4], after[4]))

# The hammer (with the drop dash, as a pack gives it): jump pressed again in a
# jump makes the character 12 pixels larger each way (object bytes +$22, +$23,
# 12 for the ball) for the 24 frames of the swing, and no longer.
out = subprocess.run(
    [HEADLESS, "--rom", ROM, "--frames", "960", "--stage", "300:0:1:0:0:7:0", "--moves", "0:A",
     "--peek-every", "1:%04X:48" % PLAYER, "--press", "900:c:14", "--press", "920:c:4"],
    capture_output=True, text=True, check=True).stdout
size = {int(l.split()[1]): bytes.fromhex(l.split()[3])[0x22:0x24] for l in out.splitlines() if l.startswith("peek ")}
swing = [f for f in sorted(size) if f >= 900 and size[f] == bytes([24, 24])]
check(size[918] == bytes([12, 12]) and len(swing) == 24 and swing[0] in (921, 922) and size[swing[-1] + 2] == bytes([12, 12]),
      "the hammer reaches 12 pixels further for 24 frames (frames %s to %s)" % (swing[:1], swing[-1:]))

knuckles = subprocess.run(
    [HEADLESS, "--rom", ROM, "--frames", "900", "--stage", "300:0:1:0:2:7:0", "--moves", "0:1",
     "--peek-every", "1:%04X:32" % PLAYER, *JUMP, *AGAIN], capture_output=True, text=True, check=True).stdout
speeds = [s32(int.from_bytes(bytes.fromhex(l.split()[3])[0x1C:0x20], "big"))
          for l in knuckles.splitlines() if l.startswith("peek 8")]
check(max(speeds) < STOMP_SPEED, "Mighty's move is not Knuckles'")

# Ray's glide (on Knuckles' slot, whose own glide the character file turns off;
# here it is still on, and the hook takes the press first): the game's glide
# state for as long as jump is held, going the way he faces at the glide's
# least speed or more. Forward held, he dives.
def glide_run(*presses):
    out = subprocess.run(
        [HEADLESS, "--rom", ROM, "--frames", "1000", "--stage", "300:0:1:0:2:7:0", "--moves", "2:4",
         "--peek-every", "1:%04X:48" % PLAYER, *JUMP, *presses], capture_output=True, text=True, check=True).stdout
    return {int(l.split()[1]): bytes.fromhex(l.split()[3]) for l in out.splitlines() if l.startswith("peek ")}


g = glide_run("--press", "866:c:120", "--press", "864:right:120")
check(g[868][7] == 0x24 and s32(int.from_bytes(g[868][0x18:0x1C], "big")) >= 0x30000 - 0x2000,
      "Ray's glide: the glide state, at 3 px a frame or more (state %02X)" % g[868][7])
check(g[880][7] == 0x24, "and it goes on while jump is held")
# The glide's frame is the pose for how his nose points (0 all the way up to
# 4 all the way down), and landing keeps his speed as ground speed.
gliding = [f for f in sorted(g) if f >= 868 and g[f][7] == 0x24]
check(all(g[f][0x28] <= 4 for f in gliding) and max(g[f][0x28] for f in gliding) == 4,
      "forward held, his nose goes down (poses %s)" % sorted({g[f][0x28] for f in gliding}))
landed = next((f for f in sorted(g) if f > 868 and g[f][7] == 0x02), None)
check(landed is not None and landed + 2 in g and int.from_bytes(g[landed + 2][0x2C:0x2E], "big", signed=True) >= 0x200,
      "landing from the glide keeps his speed (landed at %s)" % landed)
# The same to the left: he lands facing left and runs on that way.
lg = {int(l.split()[1]): bytes.fromhex(l.split()[3]) for l in subprocess.run(
    [HEADLESS, "--rom", ROM, "--frames", "1230", "--stage", "300:0:1:0:2:7:0", "--moves", "2:4",
     "--peek-every", "1:%04X:48" % PLAYER, "--press", "860:right:200", "--press", "1062:left:60",
     "--press", "1124:c:6", "--press", "1134:c:80", "--press", "1134:left:40"],
    capture_output=True, text=True, check=True).stdout.splitlines() if l.startswith("peek ")}
lg_landed = next((f for f in sorted(lg) if f > 1136 and lg[f][7] == 0x02), None)
lx = lambda f: int.from_bytes(lg[f][8:10], "big")
check(lg_landed is not None and lg[lg_landed + 4][0x25] & 8 and lx(lg_landed + 8) < lx(lg_landed) - 8,
      "gliding left, he lands facing left and runs on left (landed at %s)" % lg_landed)
up = glide_run("--press", "866:c:120")
floating = [f for f in sorted(up) if f >= 880 and up[f][7] == 0x24]
check(len(floating) > 20 and all(up[f][0x28] <= 1 for f in floating[8:]),
      "nothing held, he pulls up and floats (poses %s)" % sorted({up[f][0x28] for f in floating}))
let_go = glide_run(*AGAIN)
check(let_go[868][7] == 0x24 and let_go[874][7] != 0x24, "jump let go ends it (state %02X)" % let_go[874][7])

# Tails' flight (on Charmy's slot, which the character file gives a jump of
# the others' kind): jump pressed again in a jump flies in Charmy's flying
# state, sinking slowly; a press lifts him; he lands.
import os
import tempfile
with tempfile.TemporaryDirectory(prefix="chaotix_tails_") as chars:
    os.makedirs(os.path.join(chars, "Characters"))
    with open(os.path.join(chars, "Characters", "charmy.ini"), "w") as f:
        f.write("[Stats]\nabilities_a = 0x0D\nabilities_b = 0x42\njump = 0x0680\njump_release = 0x0400\n"
                "[Moves]\ntails_flight = 1\n")

    def tails_run(*presses):
        out = subprocess.run(
            [HEADLESS, "--rom", ROM, "--frames", "1100", "--stage", "300:0:1:0:3:7:0", "--characters",
             os.path.join(chars, "Characters"), "--peek-every", "1:%04X:48" % PLAYER, "--press", "900:c:6", *presses],
            capture_output=True, text=True, check=True).stdout
        return {int(l.split()[1]): bytes.fromhex(l.split()[3]) for l in out.splitlines() if l.startswith("peek ")}

    yv = lambda r, f: s32(int.from_bytes(r[f][0x1C:0x20], "big"))
    tj = tails_run()
    check(tj[906][7] == 0x04 and yv(tj, 906) < -0x40000, "Tails: a jump of his own (state %02X)" % tj[906][7])
    tf = tails_run("--press", "915:c:2")
    check(tf[920][7] == 0x2C, "jump again in the air flies (state %02X)" % tf[920][7])
    sinking = [yv(tf, f + 1) - yv(tf, f) for f in range(950, 960) if tf[f][7] == 0x2C and tf[f + 1][7] == 0x2C]
    # (Charmy's state adds its gravity unevenly from frame to frame: the mean.)
    check(len(sinking) >= 8 and abs(sum(sinking) / len(sinking) - 0x800) <= 0x100,
          "he sinks by 1/32 px a frame each frame on average (%s)" % [hex(d) for d in sinking])
    tl = tails_run("--press", "915:c:2", "--press", "960:c:2")
    check(yv(tl, 975) < yv(tf, 975) - 0x10000, "a press lifts him")
    tland = next((f for f in sorted(tf) if f > 920 and tf[f][7] == 0x02), None)
    check(tland is not None, "and he lands (at %s)" % tland)
    with open(os.path.join(chars, "Characters", "charmy.ini"), "w") as f:
        f.write("[Stats]\nabilities_a = 0x0D\nabilities_b = 0x42\njump = 0x0680\njump_release = 0x0400\n")
    flat = tails_run("--press", "915:c:2")
    check(flat[920][7] != 0x2C, "without the move the second press does not fly (state %02X)" % flat[920][7])

    # A pack's own flight sound (audio/sound_mods.h): a 440 Hz tone as Sounds/tails_flight.wav is heard
    # while he flies, and not before.
    import math
    import struct
    with open(os.path.join(chars, "Characters", "charmy.ini"), "w") as f:
        f.write("[Stats]\nabilities_a = 0x0D\nabilities_b = 0x42\njump = 0x0680\njump_release = 0x0400\n"
                "[Moves]\ntails_flight = 1\n")
    sounds = os.path.join(chars, "Sounds")
    os.makedirs(sounds)
    tone = b"".join(struct.pack("<h", int(12000 * math.sin(2 * math.pi * 440 * i / 44100))) for i in range(44100))
    with open(os.path.join(sounds, "tails_flight.wav"), "wb") as f:
        f.write(b"RIFF" + struct.pack("<I", 36 + len(tone)) + b"WAVEfmt " + struct.pack("<IHHIIHH", 16, 1, 1, 44100, 88200, 2, 16)
                + b"data" + struct.pack("<I", len(tone)) + tone)

    def audio(*extra):
        wav = os.path.join(chars, "out.wav")
        subprocess.run([HEADLESS, "--rom", ROM, "--frames", "960", "--stage", "300:0:1:0:3:7:0", "--characters",
                        os.path.join(chars, "Characters"), "--press", "900:c:6", "--press", "915:c:2", "--wav", wav, *extra],
                       capture_output=True, check=True)
        with open(wav, "rb") as f:
            d = f.read()
        i, rate, ch = 12, 0, 0
        while i < len(d) - 8:
            n, s = d[i:i + 4], struct.unpack("<I", d[i + 4:i + 8])[0]
            if n == b"fmt ":
                ch, rate = struct.unpack("<HI", d[i + 10:i + 16])
            if n == b"data":
                return rate, ch, struct.unpack("<%dh" % (s // 2), d[i + 8:i + 8 + s])
            i += 8 + s + (s & 1)

    # Against a silent file (which leaves out the game's own flight sound too), the difference is the tone.
    silent = os.path.join(chars, "Silent")
    os.makedirs(silent)
    with open(os.path.join(silent, "tails_flight.wav"), "wb") as f:
        f.write(b"RIFF" + struct.pack("<I", 36 + 2000) + b"WAVEfmt " + struct.pack("<IHHIIHH", 16, 1, 1, 44100, 88200, 2, 16)
                + b"data" + struct.pack("<I", 2000) + bytes(2000))
    rate, ch, plain = audio("--sounds-dir", silent)
    _, _, own = audio("--sounds-dir", sounds)
    per = rate / 60.0  # audio frames a video frame (near enough)
    def span(a, f0, f1):
        return a[int(f0 * per) * ch:int(f1 * per) * ch:ch]
    before = max(abs(x - y) for x, y in zip(span(plain, 850, 912), span(own, 850, 912)))
    check(before == 0, "the sound is not heard before he flies (largest difference %d)" % before)
    d = [x - y for x, y in zip(span(own, 925, 955), span(plain, 925, 955))]
    # The difference's strength at 440 Hz against its strength overall.
    re = sum(v * math.cos(2 * math.pi * 440 * k / rate) for k, v in enumerate(d))
    im = sum(v * math.sin(2 * math.pi * 440 * k / rate) for k, v in enumerate(d))
    power = sum(v * v for v in d)
    share = (re * re + im * im) * 2 / len(d) / power if power else 0
    check(power > 0 and share > 0.5, "while he flies the file's tone is heard (%.0f%% of the difference at 440 Hz)" % (100 * share))

sys.exit(1 if FAILED else 0)
