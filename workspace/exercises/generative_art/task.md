# Task — Generative art frame

New concepts vs. the desk badge exercise: per-pixel drawing (`epd.drawPixel(x, y, color)`)
instead of shape/text primitives, and — only if your algorithm needs it —
Floyd-Steinberg dithering to fake grayscale on a pure black/white panel
(see `workspace/sketches/test_card/test_card.ino`'s `renderPM5544Card()`
for a working reference implementation of that technique).

## Screen coordinates

400×300 pixels, Adafruit_GFX convention (same as `setCursor()` on the
badge), for `setRotation(0)` (the orientation every sketch in this project
uses):

- `(0, 0)` is the **top-left** corner.
- `x` increases **rightward**, range `0`–`399`.
- `y` increases **downward**, range `0`–`299`.

`config.h` (in `display_text/`/`test_card/`) has these as named constants
(`DISPLAY_WIDTH = 400`, `DISPLAY_HEIGHT = 300`) if you'd rather reference
those than hardcode the numbers.

## Checkpoint

Write a new sketch, from scratch (`generative_art.ino`, in this folder),
that:

1. Picks **one** algorithmic pattern to generate:
   - **Maze** — e.g. randomized depth-first-search ("recursive backtracker"),
     walls black, passages white. Naturally pure black/white.
   - **Cellular automaton** — e.g. Conway's Game of Life or a 1D rule (like
     Rule 30), run for some number of generations, live cells black. Also
     naturally binary, unless you render fading/dying cells as gray.
   - **Plotter-style lines** — e.g. a spirograph, Lissajous curve, or
     random-walk line art. Pure black/white if you just draw the line;
     grayscale if you add thickness/anti-aliasing.
2. Computes the pattern algorithmically at runtime (not a hardcoded bitmap)
   and renders it with `epd.drawPixel(x, y, color)` calls across the
   400×300 grid.
3. If (and only if) your algorithm produces continuous/grayscale values
   rather than pure black/white, dithers them down to 1-bit using the same
   Floyd-Steinberg approach `test_card.ino` uses, instead of naively
   rounding each pixel independently.
4. Reuses the same power lifecycle as the desk badge exercise: `PWR` HIGH
   → `epd.init(...)` → draw → `epd.display()` → `epd.hibernate()` → `PWR`
   LOW.

Result: an algorithmically-generated pattern rendered on the panel, which
stays up permanently once the board's panel-power rail is cut.

If you get stuck on *how the library API actually behaves* (not the
concept, just syntax), `workspace/exercises/desk_badge/desk_badge.ino` and
`workspace/sketches/test_card/test_card.ino` are both fair to peek at.
Ask for one hint only after you've made a real attempt.
