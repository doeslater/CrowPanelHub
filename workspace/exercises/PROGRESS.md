# Progress

Tracks where the learning track (`docs/firmware-learning-path.md`) is
currently at. Read this before assuming what's already been covered,
especially at the start of a session after a `/clear` — see `CLAUDE.md`'s
"Teaching mode" section for how each entry below actually gets taught.

## Current

**Idea 2 — Generative art frame** (see `docs/firmware-learning-path.md`,
Phase A #2) — not yet started. Algorithmic pattern generation on boot
(maze, cellular automaton, plotter-style lines), reusing `test_card.ino`'s
dithering approach for any grayscale effect.

## Log

- 2026-07-06 — Warm-up, GPIO output via `PWR`: wrote/compiled/uploaded
  `/workspace/basic_board/pwr_pin/pwr_pin.ino` (toggles GPIO 7 HIGH/LOW on a 1s timer,
  `Serial.println`s each transition), verified against real hardware via
  `arduino-cli monitor` — passed, no hint needed.

- 2026-07-06 — Implement RGB LED color cycling for ESP32-S3: wrote/compiled/uploaded
  `/workspace/basic_board/rgb_led/rgb_led.ino` (toggles GPIO 7 HIGH/LOW on a 1s timer,
  `Serial.println`s each transition). (not added to the warmup_theory.md)

- 2026-07-07 — moved code that runs on the barebones ESP32-S3 to the `basic_board` folder.

- 2026-07-07 — Idea 1 (Desk badge / name tag) theory given: display object +
  pin mapping, the power/init/draw/display/hibernate/power-off lifecycle,
  and the GFX text-drawing API. Scaffolded `workspace/exercises/desk_badge/`
  (`install.sh`, `cli.md`, `lifecycle.md`, `generate_flowchart.py` +
  `flowchart.png`); checkpoint (writing `desk_badge.ino`) still pending.

- 2026-07-07 — Idea 1 (Desk badge / name tag) checkpoint: wrote/compiled/
  uploaded `workspace/exercises/desk_badge/desk_badge.ino` (powers panel,
  `epd.init()`, draws badge text via `setCursor()`/`println()`, `display()`,
  `hibernate()`, powers down) — first draft had a missing semicolon and a
  stray unresolved `config.h` include, self-corrected before asking for
  review. Verified against real hardware, badge rendered and stayed up
  with power cut — passed, no hint needed.

- 2026-07-07 — Idea 1 redesign: rewrote `desk_badge.ino`'s content into a
  proper name badge (name/title/room number, multiple fonts, a divider
  line). The rewrite dropped `PWR` HIGH and `epd.init(...)` entirely and
  mismatched its font `#include`s against the `setFont()` calls — reviewed
  and flagged both without giving the fix outright; user diagnosed and
  restored both themselves. Verified against real hardware, badge renders
  correctly with the panel fully unpowered afterward — passed, one hint
  given (pointed at the missing `init()`/power sequence, not the exact fix).