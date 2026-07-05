# Firmware learning path

A suggested order for reading `sketches/`'s three sketches if you're new
to ESP32/embedded development — each one introduces a small set of new
concepts on top of the last, rather than throwing everything at you at
once. This doc is just the ordering and the "why this concept, why now"
connective tissue; the actual explanations live in each sketch's own
`README.md` (and flowchart, where one exists) — read this alongside those,
not instead of them.

**Before starting**: you should be comfortable with basic programming
(variables, functions, `if`/`else`, arrays) in any C-like language — this
path teaches ESP32/Arduino-specific and hardware concepts, not programming
from zero. You'll also want the toolchain set up — see `docs/dev-tools.md`
for installing `arduino-cli` and flashing a board.

## Stage 1 — `sketches/display_text/`

**Start here.** This sketch has no serial protocol at all — it draws a
fixed set of hardcoded text rows once, on boot, and does nothing else.
That makes it the smallest possible unit of "how do I get a pixel onto
this panel," with none of the wire-protocol complexity from Stages 2–3
mixed in.

New concepts this stage introduces:
- The Arduino sketch shape: `setup()` runs once, `loop()` runs forever.
- Initializing the GxEPD2 display driver and wiring it to the board's pins.
- Power-cycling the panel (`PWR` pin) rather than leaving it powered
  continuously.
- Drawing text and calling a full e-paper refresh (`epd.display()`) —
  and why e-paper needs an explicit refresh call at all, unlike an LCD.

Read `sketches/display_text/README.md` for the full walkthrough. Try
tinkering: change the hardcoded text, add a row, or change a font size,
then reflash and watch the panel update.

## Stage 2 — `sketches/receive_image/`

This is where the phone/PC actually gets involved. Everything from Stage
1's display calls still applies (same init/power-cycle/refresh pattern) —
this stage's new material is entirely about *getting a picture from
somewhere else onto the board* over a wire.

New concepts this stage introduces:
- Reading bytes over `Serial` (`Serial.read()`/`Serial.readBytes()`) as
  they arrive, rather than the sketch already having its data baked in.
- Framing a binary protocol: a sync/magic byte so the receiver can find
  the *start* of a message even if it starts listening mid-stream.
- Little-endian byte packing — how a 4-byte number gets reassembled from
  individual bytes sent over the wire.
- 1-bit-per-pixel bitmaps — packing 8 black/white pixels into a single
  byte, instead of one byte per pixel.
- Checksums — a cheap way for the receiver to detect a corrupted/dropped
  byte without anything fancy.
- Unix epoch timestamps, and why this firmware does zero timezone math
  (see `CLAUDE.md`'s wire protocol notes for the phone-side trick that
  makes that OK).

Read `sketches/receive_image/README.md` and its flowchart for the full
frame-by-frame walkthrough. Try tinkering: run
`send_test_frame.py`/`send_text.py` to send a real frame, then deliberately
flip a byte in a copy of the script and watch the checksum-mismatch
rejection in the serial monitor.

## Stage 3 — `sketches/test_card/`

The most involved sketch. It does everything `receive_image.ino` does
(same wire protocol, same rendering), *plus* it draws its own test pattern
on boot without any phone/PC involved — which is where the hardest new
concepts live.

New concepts this stage introduces:
- Procedural pattern generation: small per-pixel functions (`bandGrating`,
  `bandCheckerboard`, etc.) that decide a pixel's color from its
  coordinates, rather than reading pixels from anywhere.
- Floyd-Steinberg dithering — turning a grayscale value into a black/white
  dot pattern that *looks* gray from a distance, one row at a time,
  carrying rounding error forward instead of just rounding each pixel
  independently. This is the single hardest idea in any of these three
  sketches.
- A real debugging lesson: `Serial.setRxBufferSize()` must be called
  *before* `Serial.begin()`, and sized to survive a slow boot-time render —
  get this wrong and incoming frames get silently truncated. (See
  `CLAUDE.md`'s "confirm what's actually flashed" note for the war story
  behind why this matters.)

Read `sketches/test_card/README.md` and its flowchart for the full
walkthrough. Try tinkering: change one band's pattern function, or adjust
the dithering threshold (`value < 128`) and see how the test card's
gradient band changes.

## After this

At this point you've seen the full range of what's in `sketches/` today.
From here, `CLAUDE.md`'s "Open threads" section lists what's genuinely
unbuilt (Wi-Fi/BLE transports, error-state UX, a PlatformIO migration) if
you're looking for a next thing to build rather than read.
