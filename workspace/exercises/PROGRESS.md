# Progress

Tracks where the learning track (`docs/firmware-learning-path.md`) is
currently at. Read this before assuming what's already been covered,
especially at the start of a session after a `/clear` — see `CLAUDE.md`'s
"Teaching mode" section for how each entry below actually gets taught.

## Current

**Idea 1 — Desk badge / name tag** (see `docs/firmware-learning-path.md`,
Phase A #1) — not yet started.

## Log

- 2026-07-06 — Warm-up, GPIO output via `PWR`: wrote/compiled/uploaded
  `workspace/exercises/pwr_pin/pwr_pin.ino` (toggles GPIO 7 HIGH/LOW on a 1s timer,
  `Serial.println`s each transition), verified against real hardware via
  `arduino-cli monitor` — passed, no hint needed.
