# Progress

Tracks where the learning track (`docs/firmware-learning-path.md`) is
currently at. Read this before assuming what's already been covered,
especially at the start of a session after a `/clear` — see `CLAUDE.md`'s
"Teaching mode" section for how each entry below actually gets taught.

## Current

**Idea 12 — On-device info screen** (see `docs/firmware-learning-path.md`,
Phase A #3) — not yet started, no theory given yet. First GPIO *input*
exercise (a MENU button press showing the running sketch's name/build
timestamp/free heap).

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

- 2026-07-07 — Idea 2 (Generative art frame) theory given: per-pixel
  drawing (`epd.drawPixel`) vs. shape/text primitives, and Floyd-Steinberg
  dithering (reusing `test_card.ino`'s approach) for algorithms that
  produce grayscale rather than pure black/white. Presented three example
  algorithms (maze, cellular automaton, plotter-style lines) and asked
  user to pick one. Scaffolded `workspace/exercises/generative_art/`
  (`install.sh`, empty `.ino`, `task.md` incl. screen-coordinate reference).

- 2026-07-07 — Idea 2 checkpoint attempt: user first wrote a hand-composed
  fixed icon (not algorithmic — flagged this against the checkpoint's
  actual intent), then added `random()`-positioned lines seeded via
  `randomSeed(analogRead(A0))`. Bug: lines weren't actually varying
  between boots. User debugged it themselves after being pointed at
  printing the seed value and told ESP32-S3 has a real hardware RNG
  worth looking into — traced it to an earlier `randomSeed(micros())`
  call being immediately overwritten by the `analogRead(A0)` one, and
  that `analogRead(A0)` wasn't producing a useful varying value. Fix:
  dropped the `analogRead(A0)` line, kept `randomSeed(micros())` —
  hardware-verified lines now differ each boot. Also refactored 13
  repeated `drawLine()` calls into a `for` loop — good cleanup, unprompted.
  Session ended before deciding whether the fixed-icon + generative-lines
  mix counts as the checkpoint answer or needs to go further (see Current).

- 2026-07-07 — Idea 2 (Generative art frame) resolved: pushed further per
  the open question above by building an addition,
  `workspace/exercises/maze_generator/maze_generator.ino` (block-carving
  grid maze — a simplified coin-flip heuristic, not the recursive-
  backtracker algorithm from `task.md`'s theory, but a genuine
  algorithmically-generated pattern), alongside the existing fixed-icon +
  lines `generative_art.ino` — both count toward the checkpoint. Spent a
  few hours on it; one or more hints given along the way (session happened
  outside this tracked log, specifics not preserved). Hardware-verified.
  Checkpoint passed.

- 2026-07-08 — Retroactively added `solution_*.md` files for the three
  exercises already marked passed above: `desk_badge`, `basic_board/pwr_pin`,
  `basic_board/rgb_led`. Per explicit user request, not the 1.5h/2h time-gate
  in `CLAUDE.md`'s Teaching mode section — a one-time catch-up for already-
  finished work, not a standing practice. `generative_art` and the untracked
  `workspace/exercises/maze_generator/` were deliberately excluded at the
  time (not yet marked passed).

- 2026-07-09 — Added `solution_generative_art.md`, now that Idea 2 is
  marked passed above (both `generative_art.ino` and the
  `maze_generator.ino` addition).

- 2026-07-09 — Added `workspace/exercises/maze_generator/
  maze_generator.ino`: a binary-tree maze algorithm (carve into an
  already-scanned North/West neighbor — no `visited` array, no stack, no
  recursion) requested as the simplest/shortest correct alternative to the
  recursive backtracker.
