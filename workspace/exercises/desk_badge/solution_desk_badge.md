# Solution — Desk badge / name tag

Retroactive reference documentation for an exercise already marked **passed**
in `PROGRESS.md` (both the first checkpoint and the later redesign). See
`CLAUDE.md`'s Teaching mode section for why this file exists and how it's
different from code written *for* an active attempt.

## What you wrote

### High-level

`desk_badge.ino` runs the full lifecycle from `lifecycle.md` exactly once:
power the panel, initialize the controller, draw into the frame buffer,
push it to the glass, then hibernate and cut power so the badge stays
visible with the panel fully unpowered. The content itself is three
manually-positioned text elements (name, title, a divider line, room
number) rather than a loop over rows.

Two bugs worth remembering from how this actually went (per `PROGRESS.md`):

- The **first draft** had a missing semicolon and a stray unresolved
  `config.h` include — both self-corrected before review.
- The **redesign** (turning it into a proper name badge with multiple
  fonts) dropped `PWR` HIGH and `epd.init(...)` entirely, and mismatched
  its font `#include`s against the `setFont()` calls it made. Both were
  flagged without being handed the fix, and you found and restored them
  yourself. The lesson that made this stick: every `setFont(&SomeFont)`
  call needs a matching `#include <Fonts/SomeFont.h>` above it — the
  compiler doesn't catch a *missing* include as clearly as you'd expect
  when the font symbol still resolves to something else nearby.

### Line-by-line

```cpp
#include "GxEPD2_BW.h"
// #include "config.h"

// Pin definitions
#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45

// E-paper display initialization
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

void setup() {
  Serial.begin(115200);
  Serial.println("Display Controller Starting...");

  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, HIGH);

  epd.init(115200, true, 50, false);

  epd.fillScreen(GxEPD_WHITE);
  epd.setTextColor(GxEPD_BLACK);

  epd.setFont(&FreeSansBold18pt7b);
  epd.setCursor(100, 60);
  epd.print("David Later");

  epd.setFont(&FreeSansBold12pt7b);
  epd.setCursor(100, 110);
  epd.print("Project Manager");

  epd.drawFastHLine(40, 150, 320, GxEPD_BLACK);

  epd.setFont(&FreeSans12pt7b);
  epd.setCursor(260, 260);
  epd.print("Room 404");

  epd.display();
  epd.hibernate();

  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, LOW);
  Serial.println("Display Controller End...");
}

void loop() {
}
```

- **Lines 5–10**: pin definitions matching the board's fixed SPI wiring —
  identical across every sketch that drives this panel.
- **Lines 13–18**: six font headers included even though only three fonts
  (`FreeSansBold18pt7b`, `FreeSansBold12pt7b`, `FreeSans12pt7b`) are
  actually used below — leftover from earlier iteration, harmless but
  unused weight in the binary.
- **Line 20**: the display object, declared once at global scope — see
  `lifecycle.md` step 0 for why this has to exist before `setup()` runs.
- **Lines 24–25**: `Serial.begin` plus a boot log line, useful for
  confirming (via `arduino-cli monitor`) that this sketch — not something
  else — is what's actually flashed.
- **Lines 27–28**: `PWR` HIGH — energizes the panel's power rail
  (`lifecycle.md` step 2). Nothing below this line can talk to the panel.
- **Line 30**: `epd.init(...)` wakes the SSD1683 controller and configures
  a full (non-partial) refresh (`lifecycle.md` step 3).
- **Lines 32–33**: clear the in-memory frame buffer to white and set the
  drawing color — still hasn't touched the physical glass.
- **Lines 36–44**: two `setFont()`/`setCursor()`/`print()` groups — each
  font change must come *before* the `setCursor`/`print` that uses it,
  since `setFont` affects how the library measures/advances text.
- **Line 47**: `drawFastHLine(x, y, width, color)` — a raw Adafruit_GFX
  primitive for the divider line, independent of any font.
- **Lines 50–52**: a third text block for the room number, manually
  positioned near the bottom-right rather than computed from text width.
- **Line 54**: `epd.display()` — the one call that actually redraws the
  physical glass (`lifecycle.md` step 5); blocks for several seconds.
- **Line 55**: `epd.hibernate()` — sleeps the controller chip
  (`lifecycle.md` step 6), distinct from cutting power.
- **Lines 57–58**: `PWR` LOW — de-energizes the panel's power rail
  entirely (`lifecycle.md` step 7). The badge stays visible anyway because
  e-paper holds its image with zero power.
- **Line 63–64**: empty `loop()` — this sketch runs its sequence exactly
  once on boot and then idles forever; there's no receive loop like
  `test_card.ino`'s.

## Another way to do it

### High-level

The version above hardcodes one `setFont()`/`setCursor()`/`print()` block
per line of text — fine at three lines, but every new line means copying
that whole block again. `workspace/sketches/display_text/display_text.ino`
(which `task.md` explicitly says is fair to peek at) takes a different
approach: describe each row as *data* (text, font, position) in an array,
then loop over the array once. Adding, removing, or reordering badge lines
becomes an edit to the array, not a new copy-pasted block — this is the
same shape of improvement as the `for` loop you unprompted-refactored 13
`drawLine()` calls into during the generative-art exercise.

### Line-by-line

```cpp
#include "GxEPD2_BW.h"

#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45

#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

struct BadgeLine {
  const char* text;
  const GFXfont* font;
  int16_t x;
  int16_t y;
};

BadgeLine lines[] = {
  { "David Later",     &FreeSansBold18pt7b, 100, 60  },
  { "Project Manager", &FreeSansBold12pt7b, 100, 110 },
  { "Room 404",        &FreeSans12pt7b,     260, 260 },
};

void setup() {
  Serial.begin(115200);
  Serial.println("Display Controller Starting...");

  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, HIGH);

  epd.init(115200, true, 50, false);
  epd.fillScreen(GxEPD_WHITE);
  epd.setTextColor(GxEPD_BLACK);

  for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
    epd.setFont(lines[i].font);
    epd.setCursor(lines[i].x, lines[i].y);
    epd.print(lines[i].text);
  }

  epd.drawFastHLine(40, 150, 320, GxEPD_BLACK);

  epd.display();
  epd.hibernate();

  digitalWrite(PWR, LOW);
  Serial.println("Display Controller End...");
}

void loop() {
}
```

- **Only the three fonts actually used** are included — the leftover
  unused headers from your version are gone.
- **`struct BadgeLine`**: a small record type bundling everything one row
  needs — text, font pointer, and position — so one array entry fully
  describes one line of the badge.
- **`BadgeLine lines[] = { ... }`**: the badge's actual content, as data,
  declared at global scope. This is the array `display_text.ino` calls
  `rows` — same idea, renamed here to avoid confusion with the divider
  line.
- **The `for` loop in `setup()`** replaces the two repeated
  `setFont`/`setCursor`/`print` blocks with one loop body that runs once
  per array entry — `sizeof(lines) / sizeof(lines[0])` computes the
  element count at compile time, so the loop bound stays correct if a row
  is added or removed from the array.
- **The divider line and power/init/hibernate sequence are unchanged** —
  this refactor only touches *how the text rows are expressed*, not the
  lifecycle around them.

This isn't a "better" solution in any absolute sense — at exactly three
lines of text, the difference is mostly stylistic. It becomes a real
advantage once a badge needs many rows, different content per boot, or
content computed at runtime (e.g. reading a name from Serial) rather than
three fixed strings.
