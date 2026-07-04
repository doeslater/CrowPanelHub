# firmwares/

Every ESP32 Arduino sketch for this project lives here, as sibling
subfolders — a repo-root sibling of `app/` (the Android side). See
`CLAUDE.md` (repo root) for the project-wide picture; this file stays
narrowly focused on this folder.

## Sketches

| Folder | What it is |
| --- | --- |
| [`display_text/`](display_text/README.md) | A standalone reference sketch (not wired to anything) demonstrating GxEPD2 init/pin-mapping/power-cycle patterns. Deliberately kept as-is — consult it for patterns, don't evolve it. |
| [`receive_image/`](receive_image/README.md) | The milestone-1 firmware: listens on USB serial for one wire-protocol frame, validates it, and renders it with a full e-paper refresh. Hardware-verified. |
| [`test_card/`](test_card/README.md) | A second, independent firmware: draws a built-in Philips PM5544-style test card on boot (no phone/PC needed), and also accepts the same wire-protocol frames `receive_image.ino` does. Hardware-verified. |

**Only one sketch is ever flashed on the board at a time.** Don't assume
which one is currently on a board you didn't just flash yourself — each
sketch prints a distinct line early in `setup()` (see each folder's own
README for its exact boot line), readable via
`arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200` right after a
reset. This has bitten a real debugging session before — see `CLAUDE.md`'s
Commands section for the story.

## Installing a sketch

Each sketch folder has its own prerequisites, "Build and flash" section,
and `install.sh` (`./firmwares/<name>/install.sh [port]`, defaulting to
`/dev/ttyUSB0`) — see that folder's README for the specifics. All three
share one thing in common:

- **`docs/fqbn.txt`** (repo root) is the single source of truth for the
  FQBN board-option string every sketch is compiled/uploaded with — every
  `README.md` and `install.sh` here reads that same file rather than each
  hardcoding its own copy.
- **`docs/dev-tools.md`** (repo root) covers the actual prerequisites in
  full — installing `arduino-cli`, the ESP32 board package, and the
  `GxEPD2` library, plus serial port permissions — rather than repeating
  those install steps here.

## Housekeeping

**`clear-cache.sh`** clears `arduino-cli`'s compiled-sketch cache
(`~/.cache/arduino/sketches/`), which is keyed by a hash of each sketch's
source *path* — so moving/renaming a sketch folder (as happened when these
three moved here) leaves the old path's cache entry behind as orphaned
clutter. Run it any time you want a guaranteed-clean rebuild:

```bash
./firmwares/clear-cache.sh
```

It clears the whole cache unconditionally rather than trying to detect
which entries are stale — simplest, and the only cost is that the next
compile of any sketch rebuilds from scratch instead of using a cached
build.
