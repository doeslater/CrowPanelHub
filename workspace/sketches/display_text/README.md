# sketches/display_text/

**Role: reference.** Not flashed as "the" firmware — consult for GxEPD2
init/pin-mapping/power-cycle patterns only.

A standalone Arduino sketch, adapted from a third-party Elecrow example
([source](https://github.com/papercodeIN/Elecrow/tree/main/CrowPanel%20-%20ESP32%20E-Paper%20HMI%20Display%20-%204.2%20Inch/Project/Wireless_Text_Display%20-%2023-SEP-2025)).
It renders a fixed set of hardcoded text rows to the panel once, on boot —
no serial receive loop, no wire protocol, nothing wired to a phone or PC.

**This folder is a deliberate reference example, not a spike to evolve.**
It's kept here to show working GxEPD2 init/pin-mapping/power-cycle
patterns — see `CLAUDE.md` (repo root) for the project-wide picture. It
predates this repo's wire-protocol firmware (`sketches/test_card/`) and
isn't meant to grow into it; consult it for patterns when writing new
sketches, rather than editing it in place.

## Files

| File | What it is |
| --- | --- |
| `display_text.ino` | The sketch itself — see the walkthrough below. |
| `install.sh` | Compiles and uploads this sketch (`./install.sh [port]`) — has its own hardcoded FQBN, see "Build and flash" below. |
| `config.h` | Display size/margin, `GxEPD2` init parameters, font-size and row-height constants. |

## Prerequisites

Same as the other firmware in this repo (see `docs/dev-tools.md`'s
`arduino-cli` section for the actual install commands):

- `arduino-cli` installed and on `PATH`
- The ESP32 board package installed (`arduino-cli core install esp32:esp32`),
  with Espressif's board index registered first — it's a separate index
  from Arduino's default one, so a plain `core update-index` alone won't
  find it
- The `GxEPD2` library installed (`arduino-cli lib install GxEPD2`, which
  pulls in Adafruit GFX + BusIO automatically — that's also where this
  sketch's bundled fonts, e.g. `FreeMonoBold24pt7b.h`, come from)
- The board connected over USB and visible as a serial port (e.g.
  `/dev/ttyUSB0` on Linux, via its CH340 USB-serial bridge — confirm with
  `lsusb`/`arduino-cli board list`)
- On Linux, your user in the `dialout` group — if `upload` fails with a
  permissions error, see `docs/dev-tools.md`'s "Serial port permissions"
  section

## Build and flash

Same toolchain as the other firmware in this repo — see `docs/dev-tools.md`
at the repo root for install steps and more detail:

```bash
# from the repo root
arduino-cli compile --fqbn "$(cat docs/fqbn.txt)" workspace/sketches/display_text/
arduino-cli upload -p /dev/ttyUSB0 --fqbn "$(cat docs/fqbn.txt)" workspace/sketches/display_text/
```

Or, more simply, run `workspace/sketches/display_text/install.sh` (optionally passing a port,
defaulting to `/dev/ttyUSB0`) — it runs those same two commands, using its own
hardcoded copy of the FQBN so it works even without `docs/fqbn.txt` present:

```bash
./workspace/sketches/display_text/install.sh
```

This sketch has been flashed to and verified against real hardware — the
board configuration in `docs/arduino-ide-setup.md` was transcribed from a
successful Arduino IDE upload of this exact sketch (console showed `Done
uploading.` / `Hard resetting via RTS pin...`), and those same board
options translate directly into the `--fqbn` string above (`arduino-cli
compile` against this sketch has also been separately confirmed to
succeed on this machine — see `docs/dev-tools.md`).

To confirm this sketch (and not one of the others) is what's actually
flashed, read its boot-time `Serial` output
(`arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200`, then reset the
board) — it prints `Display Controller Starting...` then, once the panel
finishes drawing, `Setup completed successfully`.

## Code walkthrough

- **`DisplaySettings`/`Row`** model up to 8 text rows, each with its own
  font size, plus whole-display `border`/`invertColors` flags.
- **`epdPower()`** toggles the panel's `PWR` pin — the same power-cycle
  pattern `test_card.ino` reuses.
- **`epdInit()`** calls `epd.init(...)` with the parameters from
  `config.h`, sets rotation/text color/full-window mode.
- **`setFontSize()`/`getRowHeight()`** map the three font-size constants
  (`FONT_SIZE_SMALL`/`MEDIUM`/`LARGE`) to actual `GxEPD2` font objects and
  pixel row heights.
- **`updateDisplay()`** powers the panel on, draws the optional border and
  every configured row (advancing `yPos` by each row's height), calls
  `epd.display()`, then hibernates and powers the panel back down.
- **`setup()`** hardcodes 5 demo rows ("Hello World" / "Second line" /
  ...) and calls `updateDisplay()` once; **`loop()`** just delays — there's
  no dynamic content and nothing listens on `Serial` beyond the two
  boot-time log lines above.
