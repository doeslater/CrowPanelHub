# Task — On-device info screen (Idea 12)

New concept vs. the desk badge / generative art exercises: **GPIO input**
— reading a button instead of only driving an output pin. See
`CLAUDE.md`'s Teaching mode discussion and the theory already covered in
conversation for why `INPUT_PULLUP` + active-low reads work the way they
do. Short recap:

- MENU button is on **IO2** (per `docs/ideas.md`).
- `pinMode(MENU_PIN, INPUT_PULLUP)` in `setup()`.
- `digitalRead(MENU_PIN)` returns `LOW` when the button is *pressed*,
  `HIGH` when idle — backwards from the naive expectation, because the
  pull-up holds the pin HIGH until the button grounds it.
- No debouncing needed yet for this exercise — a single clean tap is
  enough. (Idea 10, right after this one, is where debounce correctness
  starts to matter.)

## Build-time info

- `__DATE__` and `__TIME__` are preprocessor macros — the compiler
  substitutes them with the compile date/time as string literals. No
  runtime clock involved.
- There's no built-in "sketch name" macro. Hardcode a string constant for
  it — that's the point: a label *you* control and can tell apart from
  any other sketch that might get flashed to this board.
- `ESP.getFreeHeap()` (from the ESP32 Arduino core) returns free RAM in
  bytes, at the moment you call it.

## Checkpoint

Write a new sketch, from scratch (`info_screen.ino`, in this folder),
that:

1. On boot, configures the MENU button (IO2) as an input and does
   *not* draw anything yet — stay idle in `loop()`.
2. Detects a MENU button press.
3. On a press, runs the familiar power lifecycle (`PWR` HIGH →
   `epd.init(...)` → draw → `epd.display()` → `epd.hibernate()` → `PWR`
   LOW) to show:
   - a hardcoded sketch name/label,
   - the build timestamp (`__DATE__ __TIME__`),
   - free heap (`ESP.getFreeHeap()`).
4. Goes back to idle afterward — pressing MENU again should redraw with
   a fresh (likely-unchanged, that's fine) reading.

Result: the board sits idle after boot, and a single MENU press shows
you exactly what's running on it — the practical fix for the "confirm
what's actually flashed" problem `CLAUDE.md` describes.

If you get stuck on *how the library API behaves* (not the concept, just
syntax), `workspace/exercises/desk_badge/desk_badge.ino` is fair to peek
at for the display lifecycle. Ask for one hint only after you've made a
real attempt.
