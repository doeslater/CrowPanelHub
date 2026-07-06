# Firmware learning path

This is a curriculum, not a fixed script: it lists *what* to learn and in
what order, but *how* each item actually gets taught (theory first, then a
checkpoint you write and run yourself, one hint after a real attempt if you
get stuck) is defined once in `CLAUDE.md`'s "Teaching mode" section — read
that alongside this file, not instead of it.

This supersedes an earlier three-stage version of this doc that was built
around `workspace/sketches/receive_image/` as its "Stage 2." That sketch has since
been removed (it wasn't a clear teaching vehicle), and the whole approach
of teaching-by-reading-existing-code turned out not to work anyway — this
version teaches by writing new code instead, in a separate `workspace/exercises/`
directory. `workspace/exercises/PROGRESS.md` tracks which item below is current; read
it before assuming where you are, especially after a `/clear`.

**Before starting**: you should be comfortable with basic programming
(variables, functions, `if`/`else`, arrays) in any C-like language — this
path teaches ESP32/Arduino-specific and hardware concepts, not programming
from zero. You'll also want the toolchain set up — see `docs/dev-tools.md`
for installing `arduino-cli` and flashing a board, since you'll be running
those commands yourself for every exercise.

## Prior art to consult (not exercises to redo)

`workspace/sketches/` holds two existing, working sketches — read-only reference
material, not something to reproduce by reading. They exist to show
patterns you can consult mid-exercise if you get stuck on *how the library
works*, as opposed to the conceptual hint your teaching session gives you:

- **`workspace/sketches/display_text/display_text.ino`** — `GxEPD2` init, pin-mapping,
  and the panel power-cycle pattern (`epdPower()`/`epd.init()`/
  `epd.hibernate()`) every other sketch in this repo reuses.
- **`workspace/sketches/test_card/test_card.ino`** — the wire protocol (framing,
  checksums, little-endian packing) plus Floyd-Steinberg dithering.

## Warm-up — GPIO output

Before any idea below, the smallest possible "make the real board do
something": toggle `PWR` (GPIO 7 — the pin that switches power to the panel
via a MOSFET; see the pin-definition comment in any remaining sketch's
`config.h`) on and off in a loop. This is this board's equivalent of "blink
an LED" — same GPIO-toggling concept, but on a pin that actually exists and
does something observable here, since this board has no plain LED. Theory
(what GPIO output/a MOSFET switch actually is) comes before the
checkpoint — see `CLAUDE.md`.

## The curriculum

The rest of this path is drawn from `docs/ideas.md`'s 30-idea pool, in an
order that merges that doc's two independent orderings — its Easy/Medium/Hard
learning-curve tiers, and its "natural sequence" dependency observation
(interactivity → storage → portability → connectivity) — into one
simple-to-complex sequence. Grouped by purchase requirement (per
`ideas.md`'s Part 1/Part 2 split), since that's a hard gate independent of
difficulty: you can't attempt a storage exercise without a TF card on hand,
regardless of how ready you are conceptually.

Four ideas from `ideas.md` (16–19: photo sender, fridge note,
composed-elsewhere dashboard, QR code) are intentionally **not** in this
list — they're phone/Android-app-side work needing little to no new
firmware, which isn't what this track is for. Worth doing later if/when
Android-side work resumes; see `ideas.md` directly for those.

### Phase A — board only, nothing to buy

1. **Idea 1 — Desk badge / name tag** *(Easy)*. Draw one full image on
   boot, then power down for good. The same ground `display_text.ino`
   covers, but written by you from scratch rather than read.
2. **Idea 2 — Generative art frame** *(Medium)*. Algorithmic pattern
   generation on boot (maze, cellular automaton, plotter-style lines),
   reusing `test_card.ino`'s dithering approach for any grayscale effect.
3. **Idea 12 — On-device info screen** *(Medium)*. Press MENU, show the
   running sketch's name/build timestamp/free heap. First GPIO *input*
   (a button), kept simple — a single tap, no debouncing rigor needed yet.
4. **Idea 10 — Menu/carousel** *(Medium)*. Rotary + MENU/EXIT cycle between
   stored screens. Same button-reading idea as #3, but now debouncing
   correctness actually matters.
5. **Idea 13 — Pomodoro / kitchen timer** *(Medium)*. Buttons plus tracking
   elapsed time and refreshing on a schedule instead of only on input.
6. **Idea 14 — Puzzle of the day** *(Medium)*. Same carousel-shaped
   input/output loop as #4, with real content logic (a puzzle + reveal)
   layered on top instead of just cycling static screens.
7. **Idea 15 — Simple games** *(Hard)*. Real interactive speed via partial
   refresh (`setPartialWindow()`) — the first time ghosting management
   matters, and the biggest jump in this phase.

### Phase B — Wi-Fi, nothing to buy

8. **Idea 3 — NTP clock/calendar** *(Medium)*. The classic first Wi-Fi
   project: hardcoded credentials in `config.h`, connect, fetch real time
   over NTP — fixes today's real limitation that the board only knows the
   time when a phone/PC tells it.
9. **Idea 4 — Weather panel** *(Hard)*. Fetch a public API, parse JSON,
   render the result.
10. **Idea 5 — ESP32 web server** *(Hard)*. The board serves a page;
    anyone on the network can upload an image directly, no app required.
11. **Idea 6 — Web configuration UI** *(Hard)*. A fuller settings page
    served by the board itself — what to display, refresh schedule,
    playlists (#18 below).
12. **Idea 7 — OTA firmware updates** *(Hard)*. Reflash over Wi-Fi instead
    of a cable.
13. **Idea 8 — Home Assistant / ESPHome status panel** *(Hard)*. Render
    home-automation state driven by an external system.
14. **Idea 9 — TRMNL-compatible client** *(Hard)*. Implement a real
    third-party fetch protocol (see `docs/ideas.md`/`docs/reference.md`)
    to inherit its plugin ecosystem.

### Phase C — needs a TF/microSD card

15. **Idea 22 — SD photo carousel** *(Medium)*. SPI + filesystem basics —
    load images from the card, cycle them on schedule or button press.
16. **Idea 23 — Album over the existing wire protocol** *(Medium)*. The
    phone sends frames, the board saves them to SD, rotary flips through
    them — no protocol change needed.
17. **Idea 24 — Content playlists** *(Medium)*. Generalize #15 from photos
    to heterogeneous screens (clock, then weather, then a photo) on a
    schedule.
18. **Idea 11 — E-reader** *(Hard)*. Rotary page-turns through text stored
    on the card.
19. **Idea 25 — Data logger + chart** *(Hard)*. Log GPIO-header sensor
    readings to SD, render a chart from the logged history.

### Phase D — needs a 3.7V LiPo battery

20. **Idea 26 — Battery-powered anything** *(Medium)*. Deep sleep plus
    e-paper's zero hold-power — the marquee ESP32 power trick (wake
    sources: timer, button).
21. **Idea 27 — Battery gauge** *(Medium)*. Read pack voltage via ADC, show
    a battery icon in the label strip.

### Phase E — needs a GPIO-header module

22. **Idea 28 — Sensor station** *(Medium)*. A BME280 or CO2 sensor;
    render current values plus daily min/max.
23. **Idea 29 — LoRa / Meshtastic mesh node** *(Hard)*. An SX1262 module
    turns the board into an off-grid text-message pager.
24. **Idea 30 — Electronic-shelf-label access point** *(Hard)*. The board
    as an access point pushing content to a fleet of tiny e-ink tags.

### Phase F — Bluetooth LE

25. **Idea 20 — BLE image transport** *(Hard)*. The same wire protocol
    from `CLAUDE.md`, sent over BLE instead of the USB cable.
26. **Idea 21 — Phone notification mirror** *(Hard)*. The app forwards
    phone notifications over BLE for the panel to show.

## After this

Once Phase A/B are solid, `CLAUDE.md`'s "Open threads" section (Wi-Fi/BLE
transport work, error-state UX, a PlatformIO migration) describes what's
genuinely unbuilt in the Android app and firmware toolchain, if you're
looking for non-`workspace/exercises/` work to pair with what you've learned here.
