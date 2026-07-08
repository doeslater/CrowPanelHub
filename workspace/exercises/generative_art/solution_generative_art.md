# Solution — Generative art frame

Retroactive reference documentation for an exercise now marked **passed**
in `PROGRESS.md`. See `CLAUDE.md`'s Teaching mode section for why this file
exists. Two artifacts count toward this checkpoint: the original
`generative_art.ino` (fixed icon + random lines) and a later addition,
`workspace/exercises/maze_generator/maze_generator.ino` — both are covered
below.

## What you wrote

### High-level

The checkpoint (`task.md`) asked for one algorithmically-generated pattern
picked from maze, cellular automaton, or plotter-style lines, computed at
runtime and drawn with per-pixel/primitive calls, reusing the same
power/init/draw/display/hibernate/power-off lifecycle as the desk badge.

The first pass (`generative_art.ino`) mixed a hand-placed fixed icon
(rectangle, concentric circles with cross-cutouts, a square, a triangle,
two small circles) with a genuinely algorithmic piece: 30 lines with random
endpoints, regenerated on every boot. The randomness itself needed a real
fix — an initial `randomSeed(analogRead(A0))` call was overwriting an
earlier `randomSeed(micros())` call and wasn't producing a useful varying
value, so the lines looked "random" but were actually identical every
boot. Diagnosed by printing the seed value and looking at the ESP32-S3's
hardware RNG, fixed by dropping the `analogRead(A0)` call and keeping
`randomSeed(micros())` — hardware-verified to differ each boot afterward.
Along the way, 13 repeated `drawLine()` calls were also refactored into a
`for` loop, unprompted.

Whether the fixed-icon-plus-lines mix alone satisfied "pick one algorithmic
pattern" was left as an open question. It was resolved by adding a second,
separate sketch — `maze_generator.ino` — that generates an actual
grid-based maze pattern at runtime: fill the maze area black, then walk a
grid of cells carving each cell and one random neighbor white, connecting
as it goes. Hardware-verified, with hints given along the way.

### Line-by-line — `generative_art.ino` (plotter-style lines)

```cpp
#include "GxEPD2_BW.h"
// #include "config.h"

#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45

#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

void setup() {
  randomSeed(micros());

  Serial.begin(115200);
  Serial.println("Display Controller Starting...");

  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, HIGH);

  epd.init(115200, true, 50, false);

  epd.fillScreen(GxEPD_WHITE);
  epd.setTextColor(GxEPD_BLACK);

  epd.setFont(&FreeSansBold18pt7b);
  epd.setCursor(100, 40);
  epd.print("Gen Art");

  epd.drawRect(40, 30, 320, 240, GxEPD_BLACK);

  epd.fillCircle(200, 150, 50, GxEPD_BLACK);
  epd.fillCircle(200, 150, 20, GxEPD_WHITE);

  epd.fillRect(199, 118, 3, 13, GxEPD_WHITE);
  epd.fillRect(199, 175, 3, 13, GxEPD_WHITE);
  epd.fillRect(168, 149, 13, 3, GxEPD_WHITE);
  epd.fillRect(225, 149, 13, 3, GxEPD_WHITE);

  epd.fillRect(300, 40, 40, 40, GxEPD_BLACK);

  epd.drawTriangle(320, 240, 290, 200, 350, 200, GxEPD_BLACK);

  epd.drawCircle(40, 70, 10, GxEPD_BLACK);

  epd.fillCircle(40, 120, 10, GxEPD_BLACK);

  epd.drawRect(40, 30, 320, 240, GxEPD_BLACK);

  for (int i = 0; i < 30; i++) {
    int x1 = random(40, 361);
    int y1 = random(30, 271);
    int x2 = random(40, 361);
    int y2 = random(30, 271);
    epd.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
  }

  epd.display();
  epd.hibernate();

  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, LOW);
  Serial.println("Display Controller End...");
}

void loop() {
}
```

- **Line 20**: `randomSeed(micros())` — seeds the PRNG from the number of
  microseconds since boot, which varies enough boot-to-boot to give
  different line layouts. This is the line that survived after the
  `analogRead(A0)` debugging session; there's no `analogRead` call left in
  the final version.
- **Lines 38–39, 68**: the hand-placed icon — a bordered rectangle, a
  "target" made of two filled circles with four white notches cut into it,
  a filled square, a triangle, and two small circles top-left/bottom-left.
  None of this is computed — every coordinate is a literal.
- **Lines 70–77**: the actual algorithmic piece. `random(40, 361)` and
  `random(30, 271)` pick endpoints within the bordered rectangle's
  interior (the bounds mirror the `drawRect(40, 30, 320, 240, ...)` call
  above — `40` to `40+320=360`, `30` to `30+240=270`, each `random()` call
  exclusive of its upper bound hence `361`/`271`), and `drawLine` connects
  each random pair. Looping this 30 times, rather than writing 30
  individual `drawLine` calls, is the refactor mentioned above.
- **Everything else** (power/init/display/hibernate/power-off) is the same
  lifecycle as `desk_badge.ino`, unchanged.

### Line-by-line — `maze_generator.ino` (maze addition)

```cpp
#include <Arduino.h>
#include "GxEPD2_BW.h"

#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45

#include <Fonts/FreeSansBold12pt7b.h>

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

void setup() {
  randomSeed(micros());
  Serial.begin(115200);

  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, HIGH);
  delay(50);

  epd.init(115200, true, 50, false);
  epd.fillScreen(GxEPD_WHITE);
  epd.setTextColor(GxEPD_BLACK);

  epd.setFont(&FreeSansBold12pt7b);
  epd.setCursor(50, 35);
  epd.print("Maze Generator");

  drawMaze();

  epd.display();
  epd.hibernate();

  digitalWrite(PWR, LOW);
}

void loop() {
}

void drawMaze() {
  epd.fillRect(40, 70, 320, 200, GxEPD_BLACK);

  int blockSize = 10;

  for (int y = 80; y < 260; y += blockSize * 2) {
    for (int x = 50; x < 350; x += blockSize * 2) {

      epd.fillRect(x, y, blockSize, blockSize, GxEPD_WHITE);

      if (random(0, 2) == 0) {
        if (x < 330) {
          epd.fillRect(x + blockSize, y, blockSize, blockSize, GxEPD_WHITE);
        } else {
          epd.fillRect(x, y + blockSize, blockSize, blockSize, GxEPD_WHITE);
        }
      } else {
        if (y < 240) {
          epd.fillRect(x, y + blockSize, blockSize, blockSize, GxEPD_WHITE);
        } else {
          epd.fillRect(x + blockSize, y, blockSize, blockSize, GxEPD_WHITE);
        }
      }
    }
  }
}
```

- **`epd.fillRect(40, 70, 320, 200, GxEPD_BLACK)`**: paints the entire maze
  area solid black first — every passage gets carved out of this by
  drawing white on top of it, the same "carve" idea as the theory in
  `task.md`, just implemented with filled rectangles instead of tracked
  wall state.
- **`blockSize = 10`**: each maze cell is a 10×10px white square; the outer
  loop steps `y`/`x` by `blockSize * 2 = 20`, so each iteration handles one
  "primary" cell plus room for one carved connection next to it.
- **The nested `for` loop**: walks a grid of primary cells left-to-right,
  top-to-bottom, always carving the primary cell white first
  (`fillRect(x, y, blockSize, blockSize, ...)`).
- **The `random(0, 2)` coin flip**: decides whether this cell also carves
  a passage to the right or downward — `random(0, 2)` returns 0 or 1,
  giving a 50/50 choice.
- **The edge-clamping `if`/`else` inside each branch**: `x < 330` / `y <
  240` check whether carving in the chosen direction would go past the
  maze's right/bottom edge; if so, it carves the *other* direction
  instead, so the maze never tries to draw outside its bounding rectangle.
- **What this does *not* do**, compared to the recursive-backtracker in
  `task.md`'s theory: there's no `visited` tracking and no backtracking —
  each primary cell only ever connects to exactly one neighbor, chosen
  independently of what any other cell decided. That's enough to look
  maze-like, but it doesn't guarantee every cell is reachable from every
  other cell the way a true perfect maze does (some primary cells can end
  up isolated if neither of their neighbors happened to carve toward
  them). See below for the version that does guarantee this.

## Another way to do it

### High-level

`task.md`'s theory section describes randomized depth-first search (the
"recursive backtracker"): track which cells have been visited, carve into
an *unvisited* neighbor and recurse/push onto a stack, and backtrack when
a cell has no unvisited neighbors left. Unlike the coin-flip version above,
this guarantees a **perfect maze** — every cell reachable from every other
cell, with no loops and no isolated pockets — because it only ever carves
toward cells that haven't been visited yet, and systematically backtracks
instead of giving up.

### Line-by-line

```cpp
#include <Arduino.h>
#include "GxEPD2_BW.h"

#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45

#include <Fonts/FreeSansBold12pt7b.h>

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

const int COLS = 16;
const int ROWS = 10;
const int CELL = 20;
const int ORIGIN_X = 40;
const int ORIGIN_Y = 70;

bool visited[ROWS][COLS];
// bit 0=N, 1=E, 2=S, 3=W — 1 means "wall removed" (passage carved)
uint8_t passages[ROWS][COLS];

int stackX[ROWS * COLS];
int stackY[ROWS * COLS];
int stackTop = 0;

void carveMaze() {
  memset(visited, 0, sizeof(visited));
  memset(passages, 0, sizeof(passages));

  int cx = 0, cy = 0;
  visited[cy][cx] = true;
  stackTop = 0;
  stackX[stackTop] = cx;
  stackY[stackTop] = cy;

  const int dx[4] = { 0, 1, 0, -1 };   // N, E, S, W
  const int dy[4] = { -1, 0, 1, 0 };
  const uint8_t bit[4] = { 1, 2, 4, 8 };
  const uint8_t oppositeBit[4] = { 4, 8, 1, 2 };

  while (stackTop >= 0) {
    cx = stackX[stackTop];
    cy = stackY[stackTop];

    int candidates[4];
    int count = 0;
    for (int dir = 0; dir < 4; dir++) {
      int nx = cx + dx[dir];
      int ny = cy + dy[dir];
      if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS && !visited[ny][nx]) {
        candidates[count++] = dir;
      }
    }

    if (count > 0) {
      int dir = candidates[random(0, count)];
      int nx = cx + dx[dir];
      int ny = cy + dy[dir];

      passages[cy][cx] |= bit[dir];
      passages[ny][nx] |= oppositeBit[dir];
      visited[ny][nx] = true;

      stackTop++;
      stackX[stackTop] = nx;
      stackY[stackTop] = ny;
    } else {
      stackTop--;
    }
  }
}

void drawMaze() {
  epd.fillRect(ORIGIN_X, ORIGIN_Y, COLS * CELL, ROWS * CELL, GxEPD_BLACK);

  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int px = ORIGIN_X + x * CELL;
      int py = ORIGIN_Y + y * CELL;

      epd.fillRect(px + 2, py + 2, CELL - 4, CELL - 4, GxEPD_WHITE);

      if (passages[y][x] & 2) {  // East
        epd.fillRect(px + CELL - 2, py + 2, 4, CELL - 4, GxEPD_WHITE);
      }
      if (passages[y][x] & 4) {  // South
        epd.fillRect(px + 2, py + CELL - 2, CELL - 4, 4, GxEPD_WHITE);
      }
    }
  }
}

void setup() {
  randomSeed(micros());
  Serial.begin(115200);

  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, HIGH);
  delay(50);

  epd.init(115200, true, 50, false);
  epd.fillScreen(GxEPD_WHITE);
  epd.setTextColor(GxEPD_BLACK);

  epd.setFont(&FreeSansBold12pt7b);
  epd.setCursor(50, 40);
  epd.print("Maze Generator");

  carveMaze();
  drawMaze();

  epd.display();
  epd.hibernate();

  digitalWrite(PWR, LOW);
}

void loop() {
}
```

- **`COLS`/`ROWS`/`CELL`**: a 16×10 grid of 20px cells fits inside the
  320×200 maze area used by the original, same footprint as before.
- **`visited[ROWS][COLS]`**: exactly the tracking structure `task.md` calls
  for — one bool per cell, whether it's been reached yet.
- **`passages[ROWS][COLS]`**: one byte per cell, 4 bits used (N/E/S/W) to
  record which walls have been carved open — the "wall-state array"
  `task.md` leaves as an implementation choice.
- **`stackX`/`stackY`/`stackTop`**: a manual stack (arrays plus an index)
  standing in for recursion — carving 160 cells deep would risk overflowing
  the ESP32's call stack with true recursion, so the algorithm is written
  iteratively instead, exactly mirroring the "push"/"pop" language in
  `task.md`'s theory.
- **`carveMaze()`**: starts at cell `(0,0)`, marks it visited, pushes it.
  Each iteration looks at the *current* (top-of-stack) cell's unvisited
  neighbors; if there's at least one, it picks one at random, records the
  passage in **both** cells' `passages` bytes (`bit[dir]` on the current
  cell, `oppositeBit[dir]` on the neighbor — a passage always has two
  sides), marks the neighbor visited, and pushes it. If there are no
  unvisited neighbors, it pops the stack instead — this is the
  "backtrack to the previous cell and try again" step.
- **The loop ends when `stackTop < 0`**: every reachable cell has been
  visited and the stack has fully unwound — matches `task.md`'s "repeat
  until the stack is empty."
- **`drawMaze()`**: draws each cell as a black-background square with a
  smaller white square carved into its center, then — only where
  `passages[y][x]` actually records an opening — extends that white area
  through the East or South wall into the neighboring cell. (Only
  East/South are drawn per-cell rather than all four directions, since
  each interior wall is shared with exactly one already-processed
  neighbor — drawing North/West here would just redraw a passage the
  neighboring cell already carved from its own East/South side.)

Both versions look similar on the glass — dense corridors of black and
white — but only this one is guaranteed to be a single connected,
loop-free maze with a real solution path between any two cells, because
carving is restricted to genuinely unvisited cells and every dead end
gets backtracked rather than silently left as-is.
