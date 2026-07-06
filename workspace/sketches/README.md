# workspace/sketches/

Every ESP32 Arduino sketch for this project lives here, as sibling
subfolders — under `workspace/`, a repo-root sibling of `app/` (the Android
side) that also holds the `exercises/` teaching curriculum. See
`CLAUDE.md` (repo root) for the project-wide picture; this file stays
narrowly focused on this folder.

**New to ESP32/embedded development?** See
[`docs/firmware-learning-path.md`](../../docs/firmware-learning-path.md) for
the current learning curriculum — it's now a hands-on `workspace/exercises/` track
rather than a reading order across these sketches, since reading-only
teaching didn't work (see `CLAUDE.md`'s "Teaching mode" section).

## Sketches

| Folder | Role | What it is |
| --- | --- | --- |
| [`display_text/`](display_text/README.md) | Reference | A standalone reference sketch (not wired to anything) demonstrating GxEPD2 init/pin-mapping/power-cycle patterns. Deliberately kept as-is — consult it for patterns, don't evolve it. |
| [`test_card/`](test_card/README.md) | Test tooling | Draws a built-in Philips PM5544-style test card on boot (no phone/PC needed), and also accepts wire-protocol frames over Serial afterwards. Hardware-verified. |

`workspace/sketches/receive_image/`, the original milestone-1 firmware, was removed
— it wasn't a clear teaching vehicle. `test_card/` covers the same
wire-protocol-frame-rendering role today.

**Only one sketch is ever flashed on the board at a time.** Don't assume
which one is currently on a board you didn't just flash yourself — each
sketch prints a distinct line early in `setup()` (see each folder's own
README for its exact boot line), readable via
`arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200` right after a
reset. This has bitten a real debugging session before — see `CLAUDE.md`'s
Commands section for the story.

## Installing a sketch

Each sketch folder has its own prerequisites, "Build and flash" section,
and `install.sh` (`./workspace/sketches/<name>/install.sh [port]`, defaulting to
`/dev/ttyUSB0`) — see that folder's README for the specifics. Both share
one thing in common:

- **`docs/fqbn.txt`** (repo root) holds the FQBN board-option string every
  sketch is compiled/uploaded with, for manual/documentation use. Each
  `install.sh` hardcodes its own copy of this same string rather than
  reading the file, so a sketch folder still works if copied out of the
  repo on its own — if the FQBN ever changes, update all three copies.
- **`docs/dev-tools.md`** (repo root) covers the actual prerequisites in
  full — installing `arduino-cli`, the ESP32 board package, and the
  `GxEPD2` library, plus serial port permissions — rather than repeating
  those install steps here.

## Housekeeping

**`clear-cache.sh`** clears `arduino-cli`'s compiled-sketch cache
(`~/.cache/arduino/sketches/`), which is keyed by a hash of each sketch's
source *path* — so moving/renaming a sketch folder (as happened when these
sketches moved here) leaves the old path's cache entry behind as orphaned
clutter. Run it any time you want a guaranteed-clean rebuild:

```bash
./workspace/sketches/clear-cache.sh
```

It clears the whole cache unconditionally rather than trying to detect
which entries are stale — simplest, and the only cost is that the next
compile of any sketch rebuilds from scratch instead of using a cached
build.
