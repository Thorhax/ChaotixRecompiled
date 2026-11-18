#!/usr/bin/env python3
"""The extra moves (runtime/moves.h), measured in the game.

Mighty in Botanic Base 1: a jump, and jump pressed again in the air.
  - with the stomp: he stops dead (X velocity 0), falls at the stomp's speed or
    more, lands sooner than the jump alone would, and hops (a short jump);
  - with left or right held, or without the move, the second press does
    nothing of the kind.
Usage: test_moves.py <chaotix_headless> <rom>
"""
import subprocess
import sys

HEADLESS, ROM = sys.argv[1], sys.argv[2]
PLAYER = 0xAD48  # the player's object in Botanic Base 1
STOMP_SPEED = 0x00099900
FAILED = []


def check(ok, what):
    print(("ok   " if ok else "FAIL ") + what)
    if not ok:
        FAILED.append(what)


def s32(v):
    return v - (1 << 32) if v & 0x80000000 else v


def run(*extra):
    """Per frame: (state, x velocity, y velocity, ground speed) of the player's object."""
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
                                 int.from_bytes(b[0x2C:0x2E], "big", signed=True))
    return frames


JUMP = ["--press", "850:c:12"]
AGAIN = ["--press", "866:c:3"]


def landing(frames, after):
    return next(f for f in sorted(frames) if f > after and frames[f][0] == 0x02)


plain = run(*JUMP)
check(plain[860][0] == 0x04, "a jump is state 04")
plain_landing = landing(plain, 855)

stomp = run("--moves", "0:1", *JUMP, *AGAIN)
check(stomp[868][0] == 0x08 and stomp[868][1] == 0 and stomp[868][2] >= STOMP_SPEED,
      "jump again in the air: straight down at the stomp's speed (y velocity %#x)" % stomp[868][2])
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

held = run("--moves", "0:1", "--press", "850:c:12", "--press", "864:right:8", "--press", "866:c+right:3")
check(all(held[f][2] < STOMP_SPEED for f in range(866, 880)), "left or right held: no stomp")

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
check(left[left_landing + 2][0] == 0x06 and left[left_landing + 2][3] <= -(DASH_SPEED - 0x100),
      "left held: the dash goes left (ground speed %d)" % left[left_landing + 2][3])

knuckles = subprocess.run(
    [HEADLESS, "--rom", ROM, "--frames", "900", "--stage", "300:0:1:0:2:7:0", "--moves", "0:1",
     "--peek-every", "1:%04X:32" % PLAYER, *JUMP, *AGAIN], capture_output=True, text=True, check=True).stdout
speeds = [s32(int.from_bytes(bytes.fromhex(l.split()[3])[0x1C:0x20], "big"))
          for l in knuckles.splitlines() if l.startswith("peek 8")]
check(max(speeds) < STOMP_SPEED, "Mighty's move is not Knuckles'")

# Ray's glide (on Knuckles' slot, whose own glide the character file turns off;
# here it is still on, and the hook takes the press first): the game's glide
# state, going the way he faces at the glide's least speed or more.
glide = subprocess.run(
    [HEADLESS, "--rom", ROM, "--frames", "900", "--stage", "300:0:1:0:2:7:0", "--moves", "2:4",
     "--peek-every", "1:%04X:48" % PLAYER, *JUMP, *AGAIN], capture_output=True, text=True, check=True).stdout
g = {int(l.split()[1]): bytes.fromhex(l.split()[3]) for l in glide.splitlines() if l.startswith("peek ")}
check(g[868][7] == 0x24 and s32(int.from_bytes(g[868][0x18:0x1C], "big")) >= 0x30000 - 0x2000,
      "Ray's glide: the glide state, at 3 px a frame or more (state %02X)" % g[868][7])
check(g[880][7] == 0x24, "and it goes on with jump let go")

sys.exit(1 if FAILED else 0)
