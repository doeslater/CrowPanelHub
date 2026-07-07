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

## Already attempted: plotter-style lines

`generative_art.ino` currently has a first pass at this option: a `for`
loop of `random()`-positioned lines, seeded from `micros()` so it differs
each boot (hardware-verified). Whether that (mixed with a hand-placed
fixed icon) counts as the finished checkpoint, or whether to push it
further, is still an open call — see `PROGRESS.md`. The two theory
sections below are for the **other** two options, for whenever you want
to try them instead of (or alongside) the lines version.

## Theory — Maze (randomized depth-first search / "recursive backtracker")

Represent the maze as a **grid of cells** (not individual pixels) — e.g.
20×15 cells at 20 pixels each, filling the 400×300 panel. Each cell has up
to 4 neighbors (N/S/E/W) and starts fully walled off from all of them.
The algorithm "carves" passages between cells until every cell is
reachable:

1. Pick a random starting cell, mark it visited, push it onto a stack.
2. Look at the current cell's unvisited neighbors. If there's at least
   one: pick one at random, **remove the wall** between the current cell
   and that neighbor (this is the actual maze-carving step), mark the
   neighbor visited, push it onto the stack, and make it the new current
   cell.
3. If there are no unvisited neighbors: pop the stack (backtrack to the
   previous cell) and try again from there.
4. Repeat until the stack is empty — every cell has been visited, and the
   maze is fully carved.

Track visited/unvisited with a 2D array (e.g. `bool visited[15][20]`).
Whether a wall exists between two specific neighboring cells needs its
own storage too (e.g. 4 bits per cell, one per direction, or a
separate wall-state array) — you decide the representation.

Rendering is naturally pure black/white: draw the full grid of walls
first (every cell boundary black), then as you carve each passage, draw
over that specific wall segment in white. No dithering needed.

## Theory — Cellular automaton

Two different shapes this can take — pick whichever sounds more
interesting:

**1D elementary CA (e.g. Rule 30).** Each row of the panel is one
"generation." A cell's next-generation state depends on exactly 3 cells
from the row above it (itself and its two horizontal neighbors) — a rule
number (0–255) encodes, for each of the 8 possible 3-cell patterns,
whether the result is alive or dead (this is exactly a lookup table:
`{111, 110, 101, 100, 011, 010, 001, 000} -> {0 or 1 each}`, read off
the rule number's binary representation). Start row 0 with a single black
"cell" in the middle, everything else white, then compute each subsequent
row from the previous one using the rule and draw it. Choose a cell size
(1 pixel per cell for a fine fractal-like result across all 300 rows, or
a bigger block like 4×4 pixels per cell for a chunkier look with fewer
generations). Naturally pure black/white — no dithering needed.

**2D Game of Life.** Each cell is alive or dead based on its 8 neighbors
in the previous generation: a live cell with 2 or 3 live neighbors
survives, a dead cell with exactly 3 live neighbors becomes alive,
everything else dies/stays dead. You need **two** grids (current
generation and next generation) since every cell's next state depends on
the *current*, not-yet-updated state of its neighbors — update into the
second grid, then swap. Seed generation 0 randomly (or with a known
pattern like a glider), run some fixed number of generations, then render
whichever generation you stop on. Also naturally binary — dithering would
only come into play if you chose to render "how recently a cell died" as
a fading gray trail, which is optional flourish, not required.

## Checkpoint

Write a new sketch, from scratch (`generative_art.ino`, in this folder),
that:

1. Picks **one** algorithmic pattern to generate — maze, cellular
   automaton, or plotter-style lines (see the theory sections above for
   the first two; the lines option is already underway).
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
