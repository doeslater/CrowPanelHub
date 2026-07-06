# sketches/test_card/

**Role: test tooling.** Independent boot self-test plus wire-protocol
receiver.

A second, independent firmware for this board (alongside `display_text/`).
It draws a Philips PM5544-style broadcast test card (checkered border,
dithered gray-grid background, five-band circle) directly on boot, no
phone or PC needed, and also accepts wire-protocol frames over Serial
afterwards. It was originally built as a sibling of
`sketches/receive_image/receive_image.ino` (since removed — it wasn't a
clear teaching vehicle) with the same wire-protocol handling, plus its own
boot-time test card on top; today it's the only sketch here that
implements the wire protocol.

See `CLAUDE.md` (repo root) for the project-wide picture. This file stays
narrowly focused on *this folder's* code.

**More than one sketch exists for this board, and it only ever runs
whichever one was flashed last.** Don't assume which one is on a board you
didn't just flash yourself — check its boot output (see "Build and flash"
below). This distinction is exactly what bit an earlier debugging session:
see `CLAUDE.md`'s Commands section for the full story.

## Files

| File | What it is |
| --- | --- |
| `test_card.ino` | The firmware itself — see the walkthrough below. |
| `install.sh` | Compiles and uploads this sketch (`./install.sh [port]`) — has its own hardcoded FQBN, see "Build and flash" below. |
| `config.h` | Pin numbers, display size, and the wire-protocol constants described in `CLAUDE.md`, kept as its own copy per this repo's convention of self-contained sketch folders. Also the single source of truth for these constants on the Python side — see `config_h.py`. |
| `config_h.py` | Parses `config.h` so `generate_test_pattern.py`/`render_preview.py` don't hardcode a value `config.h` already defines. |
| `serial_sender.py` | Stdlib-only helpers (board reset via DTR/RTS, frame building) imported by `generate_test_pattern.py`/`send_checkerboard.py`, not run directly — its own copy per this repo's self-contained-sketch-folder convention (same reasoning as `config.h` above). |
| `send_checkerboard.py` | Sends a plain checkerboard test frame, the simplest smoke test. |
| `generate_test_pattern.py` | Builds the same PM5544-style card as a full-canvas (no reserved label strip) payload and sends it to the board over the wire protocol — see "Sending a test frame" below. |
| `render_preview.py` | Renders a PNG (`boot_preview.png`) of exactly what `test_card.ino`'s own boot self-test draws, without needing hardware — see "Previewing without hardware" below. |
| `CompletePattern.jpg` | Reference photo of a real PM5544 card, used to measure `generate_test_pattern.py`'s band proportions. |
| `preview.png` / `boot_preview.png` | Regenerable outputs of the two scripts above (gitignored). |
| `generate_flowchart.py` | Regenerates `flowchart.png` below. |
| `flowchart.png` | The diagram in the code walkthrough section. |

## Prerequisites

The same `arduino-cli` prerequisites as every sketch in this repo (see
`docs/dev-tools.md`'s `arduino-cli` section for the actual install commands):

- `arduino-cli` installed and on `PATH`
- The ESP32 board package installed (`arduino-cli core install esp32:esp32`),
  with Espressif's board index registered first — it's a separate index
  from Arduino's default one, so a plain `core update-index` alone won't
  find it
- The `GxEPD2` library installed (`arduino-cli lib install GxEPD2`, which
  pulls in Adafruit GFX + BusIO automatically)
- The board connected over USB and visible as a serial port (e.g.
  `/dev/ttyUSB0` on Linux, via its CH340 USB-serial bridge — confirm with
  `lsusb`/`arduino-cli board list`)
- On Linux, your user in the `dialout` group — if `upload` fails with a
  permissions error, see `docs/dev-tools.md`'s "Serial port permissions"
  section

## Build and flash

Same toolchain as every sketch in this repo — see `docs/dev-tools.md` at
the repo root for install steps and more detail:

```bash
# from the repo root
arduino-cli compile --fqbn "$(cat docs/fqbn.txt)" sketches/test_card/
arduino-cli upload -p /dev/ttyUSB0 --fqbn "$(cat docs/fqbn.txt)" sketches/test_card/
```

Or, more simply, run `sketches/test_card/install.sh` (optionally passing a port,
defaulting to `/dev/ttyUSB0`) — it runs those same two commands, using its own
hardcoded copy of the FQBN so it works even without `docs/fqbn.txt` present:

```bash
./sketches/test_card/install.sh
```

Once flashed, the panel should show the built-in PM5544-style test card
within a few seconds of power-on/reset, with no serial connection needed.

To confirm which sketch is actually on a board, read its boot-time
`Serial` output (`arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200`,
then reset the board):

| Sketch | Boot line |
| --- | --- |
| `test_card.ino` | `test_card ready, waiting for frames` |
| `display_text.ino` | `Display Controller Starting...` |

If a board instead prints `Receive Image Controller Starting...`, it's
still running an old build of `receive_image.ino` from before that sketch
was removed from this repo — firmware persists on a board independent of
what's in git (see `CLAUDE.md`'s "confirm what's actually flashed" note).

## Running the self-test

Since this sketch draws its built-in test card on every boot with no frame
needing to be sent, "running" it needs no script at all: `arduino-cli
upload` (see "Build and flash" above) already resets the board as part of
flashing, so the self-test starts right away. To watch the boot output
yourself and confirm it came back up as `test_card.ino` (not some other
sketch — see the boot-line table above), use `arduino-cli monitor`
directly:

```bash
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

Then reset the board (power-cycle it, or press its reset button) and watch
for the `"test_card ready, waiting for frames"` line.

## Sending a test frame

`generate_test_pattern.py` builds the same PM5544-style card and sends it
over the wire protocol via `--send`. It's a full-canvas render (circle
centered on all 300 rows, no reserved label strip) — that's fine because
`renderPayload()` draws whatever it's handed edge-to-edge and clears the
label strip itself afterwards, so the received frame and the boot self-test
don't need pixel-identical layouts:

```bash
python3 sketches/test_card/generate_test_pattern.py --send
```

This folder's own `send_checkerboard.py` also works if you just want a
plain checkerboard instead:

```bash
python3 sketches/test_card/send_checkerboard.py
```

A real send should end with this sketch printing `frame ok, <label>`, and
the panel should visibly redraw with a centered "Last updated: ..." label
along the bottom.

## Previewing without hardware

`render_preview.py` is a line-for-line Python port of `test_card.ino`'s own
boot self-test drawing code (not `generate_test_pattern.py`'s full-canvas
layout — the two intentionally differ, see above), useful for checking the
card or the "last updated" label placement without flashing anything:

```bash
python3 sketches/test_card/render_preview.py                          # boot self-test, no label
python3 sketches/test_card/render_preview.py "Last updated: 2026-07-04 15:35:31"
```

Saves to `boot_preview.png`, deliberately a different filename from
`generate_test_pattern.py`'s `preview.png` since they render different
images and would otherwise silently overwrite each other.

## Code walkthrough

![Flowchart of test_card.ino's lifecycle: setup() enlarges the Serial RX buffer, powers on the panel, and renders a built-in PM5544-style self-test card before entering loop(), which waits for a byte, hands off to receiveFrame() to scan for the sync byte and validate a wire-protocol frame's header/length/payload/checksum in sequence -- bailing out to a shared error-log box on any failure -- and on success formats a timestamp label and renders the received frame before looping back to wait for the next one.](flowchart.png)

Read this alongside `test_card.ino` — the inline comments there go into
more depth on individual pieces than the diagram does. The wire-protocol
half (`receiveFrame()`/`loop()`/`renderPayload()`) implements the protocol
described in `CLAUDE.md`: magic byte, length, timestamp, payload,
checksum, in that order, with no reply ever sent. What's unique to this
sketch is how it builds its boot pattern:

- **`computeBandBounds()`** works out the five circle bands' y-ranges once
  at boot (`BAND_FRACTIONS`, rounded, with the last band absorbing any
  leftover rounding error).
- **`bandTopSplit()`/`bandGrating()`/`bandCheckerboard()`/`bandGradient()`/`bandBottomSplit()`**
  each answer "what grayscale value belongs at this (x, y)?" for one band
  of the circle.
- **`borderCellColor()`** answers the same question for the checkered
  border ring.
- **`renderPM5544Card()`** builds the whole card and dithers it straight
  into the output buffer, one row at a time, with no separate grayscale
  canvas ever allocated — Floyd-Steinberg error diffusion only needs each
  pixel's value once, in the same order this loop already visits them, so
  it dithers on the fly, carrying pending error forward in two small
  400-entry row buffers (`errCurr`/`errNext`). See the function's comment
  for the full explanation.
- **`drawGridLines()`** punches white grid lines through the dithered
  background afterwards, skipping anywhere inside the circle.
- **`BUILD_TIMESTAMP`** is a compile-time constant (`__DATE__`/`__TIME__`,
  filled in by the compiler) shown at boot in place of a real clock value
  — the ESP32 has no RTC, so it can't know the *current* time until a
  frame supplies one, but it always knows when it was compiled.
- **`renderPayload()`** centers whichever label it's given (the boot
  timestamp or a real "Last updated" one) using `getTextBounds()` to
  measure the rendered width first, rather than drawing at a fixed x.

## Regenerating the flowchart

Hand-drawn with Pillow rather than `graphviz`/`mermaid-cli`, since neither
is installed on the reference dev machine and installing one needs a
system package (`docs/android-app.md`'s flowchart uses the same approach,
for the same reason). Edit the script and rerun it if `test_card.ino`'s
logic changes:

```bash
python3 sketches/test_card/generate_flowchart.py   # from the repo root
```
