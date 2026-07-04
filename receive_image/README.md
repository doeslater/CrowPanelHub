# receive_image/

This is the milestone-1 firmware: it listens on USB serial for one
length-prefixed bitmap frame, validates it, and renders it to the e-paper
panel with a full refresh. It's fire-and-forget — the ESP32 never sends
anything back to the phone/PC beyond `Serial.println()` log lines, which
exist for a human watching a serial monitor, not for the sender to parse.

See `CLAUDE.md` (repo root) for the project-wide picture — current milestone,
why USB serial first, the Android side. This file stays narrowly focused on
*this folder's* code.

## Files

| File | What it is |
| --- | --- |
| `receive_image.ino` | The firmware itself — see the walkthrough below. |
| `config.h` | Pin numbers, display size, and wire-protocol constants shared by the whole sketch. |
| `send_test_frame.py` | Stdlib-only Python script standing in for the Android app during bring-up — resets the board and sends a checkerboard test frame. |
| `send_text.py` | Same idea, but rasterizes text with Pillow instead of a checkerboard. |
| `generate_flowchart.py` | Regenerates `flowchart.png` below (`python3 generate_flowchart.py`). |
| `flowchart.png` | The diagram in the next section. |

## Wire protocol

One frame, sent phone/PC → ESP32:

```
[1-byte magic 0xA5][4-byte LE length][4-byte LE unix epoch seconds][payload][1-byte checksum]
```

- **magic**: lets the receiver find the *start* of a frame even if it started
  listening mid-transmission (e.g. after its own reboot) — see `config.h`.
- **length**: always `PAYLOAD_SIZE` (15,000 = 400×300 pixels ÷ 8 bits/byte).
  The panel is 1-bit (black/white only), so each pixel is a single bit,
  packed MSB-first within each row.
- **epoch seconds**: wall-clock time for the "last updated" label — the
  ESP32 has no RTC/NTP of its own, so the sender supplies it. See the
  timestamp note in `config.h` and in `CLAUDE.md`'s wire protocol section
  for how the Android app fakes local time with zero firmware-side timezone
  math.
- **checksum**: an 8-bit sum (mod 256) of the payload bytes, so a corrupted
  transfer gets rejected instead of drawn.

## Build and flash

Built/flashed with `arduino-cli` (see `docs/dev-tools.md` at the repo root
for install steps and more detail — this is just the two commands actually
run against this sketch):

```bash
# from the repo root
arduino-cli compile --fqbn "esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=default,PSRAM=opi,PartitionScheme=default" receive_image/
arduino-cli upload -p /dev/ttyUSB0 --fqbn "esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=default,PSRAM=opi,PartitionScheme=default" receive_image/
```

`/dev/ttyUSB0` is this board's port on Linux with its CH340 USB-serial
bridge (confirmed via `lsusb`) — check `arduino-cli board list` if yours
shows up differently. If `upload` fails with a permissions error, see
`docs/dev-tools.md`'s "Serial port permissions" section (you likely need to
be in the `dialout` group).

**Before assuming a bug is in a payload/script, confirm this is actually
what's flashed** — read the board's boot-time `Serial` output (e.g.
`arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200`, then reset the
board). This sketch prints `"Receive Image Controller Starting..."` right
at boot; if you see anything else, a different sketch is what's actually
running. See `CLAUDE.md`'s Commands section for why this check matters.

## Sending a test frame

Once the firmware above is flashed, either Python script builds a frame
matching the wire protocol above and sends it over the same port:

```bash
python3 receive_image/send_test_frame.py                 # checkerboard pattern
python3 receive_image/send_text.py                       # built-in default text lines
python3 receive_image/send_text.py "Hello" "from Python"  # or your own lines
```

Both reset the board first (a DTR/RTS pulse), print whatever the board logs
back over `Serial` (the messages named in the code walkthrough below), and
need no install beyond the standard library for `send_test_frame.py` —
`send_text.py` additionally needs Pillow (`pip install pillow`) to
rasterize text. A real send should end with `Frame received, rendering...`
followed by `Render complete`, and the panel should visibly update.

## Code walkthrough

![Flowchart of receive_image.ino's frame lifecycle: loop() waits for the sync byte, handleFrame() reads and validates length/timestamp/payload/checksum in sequence bailing out to a shared error-log box on any failure, and a successful frame flows into renderFrame()'s power-on/init/draw/refresh/hibernate/power-off sequence before looping back to wait for the next frame.](flowchart.png)

Read this alongside `receive_image.ino` — the inline comments there go
into more depth on individual API calls (what `epd.hibernate()` does, why
`frameBuffer` is a plain global array, how the little-endian byte
reassembly works, etc.) than the diagram does.

- **`setup()`** runs once at boot: starts `Serial`, sets its read timeout,
  and prints a startup line. It deliberately does *no* panel work — no
  `display.init()`, no drawing — so `loop()` (and therefore serial
  reception) is ready almost immediately after power-on.
- **`loop()`** just watches for the one sync byte (`FRAME_MAGIC`,
  `0xA5`). Anything else read is silently discarded, which keeps the
  reader able to resynchronize if it ever starts mid-frame or sees noise.
- **`handleFrame()`** runs once a sync byte has been found. It reads the
  rest of a frame in wire order — length, timestamp, payload, checksum —
  bailing out (logging one specific message, returning) the moment
  anything doesn't check out: a read that times out, a length that isn't
  exactly `PAYLOAD_SIZE`, or a checksum that doesn't match. No reply is
  ever sent for a bad frame; the sender has no way to know except by not
  seeing the panel update.
- **`renderFrame()`** only runs after every check above passes. It's the
  full power cycle for one frame: power the panel on, initialize/reset it,
  draw the bitmap plus the "last updated" label into its in-memory buffer,
  push that to the physical panel (`display()` — the one slow, blocking
  call that actually changes what you see), then hibernate the controller
  and cut its power before returning to `loop()`.
- **`checksumOf()`** and **`readExact()`** are small helpers used by
  `handleFrame()` — see their comments in the source for exactly how each
  works.

## Regenerating the flowchart

No diagramming tool (`graphviz`, `mermaid-cli`) is installed on the
reference dev machine, and installing one needs a system package. So
`generate_flowchart.py` hand-draws the diagram with Pillow instead — the
same approach `test_screen/generate_test_pattern.py` uses for its bitmap.
Edit the script and rerun it if `receive_image.ino`'s logic changes:

```bash
python3 receive_image/generate_flowchart.py   # from the repo root, like the commands above
```
