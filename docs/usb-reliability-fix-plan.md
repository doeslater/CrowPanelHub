# USB send reliability fix — agreed plan (not yet built)

Status: **planned, not implemented**. Estimated at ~3-4 hours of work; user wants a
dedicated ~5-hour slot before starting. This doc is the durable record of what was
diagnosed and agreed, so the work can resume cold.

## What's broken (diagnosed 2026-07-10)

Two independent problems, found via a live debugging session (phone connected over
adb alongside its USB-serial link to the CrowPanel board):

1. **Flaky phone-side USB-C OTG link.** The Diagnostics screen showed:
   `Connect failed: No USB serial device found` → `Connecting...` → `Connected` →
   `Sending image (15000 bytes)...` → `Send failed: Error writing 32 bytes at
   offset 0 of total 15010 after 0msec, rc=-1`.

   `adb logcat`'s `UsbPortManager` events showed the port's `connected` status
   flapping `true`/`false` continuously every ~10-40 seconds for the whole
   session, with `currentMode` alternating between `dfp` (phone as USB host,
   talking to the board) and `ufp` (phone as USB device, as if plugged into a
   charger). No overcurrent/thermal/moisture-protection events were logged. This
   is the classic signature of a cheap/passive USB-C OTG adapter or cable
   without a proper CC pull-up resistor, causing the phone's port controller to
   repeatedly renegotiate its role. The observed send failure landed exactly
   inside one of these disconnected windows — the USB link was physically gone
   at that instant, hence the instant `rc=-1` at byte offset 0.

   `UsbSerialTransport.kt`'s connect/write logic itself has no bug — this is a
   hardware/cable-level issue upstream of the app and the board.

2. **Wrong firmware was flashed.** Resetting the board (DTR/RTS pulse, same
   trick `serial_sender.py` uses) and reading its boot banner showed
   `info_screen starting...` — not `test_card ready, waiting for frames`.
   `workspace/exercises/info_screen/info_screen.ino` (a teaching-curriculum
   exercise sketch) was flashed instead of
   `workspace/sketches/test_card/test_card.ino` (the milestone-1 firmware that
   actually implements the wire protocol). `info_screen.ino`'s `loop()` only
   polls a physical button (`handleMenuPress()`) and never reads `Serial` at
   all — any bytes the phone sends just sit unread. From the phone's side, a
   successful write to `info_screen.ino` looks identical to a successful write
   to `test_card.ino`, since the wire protocol is currently one-directional and
   fire-and-forget (the ESP32 never replies).

## Agreed scope

Both problems get addressed, with these boundaries explicitly discussed and
agreed:

- **In scope:** Android-side retry/reconnect logic for the flaky link, a small
  `test_card.ino` change reusing its *existing* text output as a de facto ack
  (no new binary protocol), a board-reset button, and real-time status text in
  the UI.
- **Explicitly deferred, not building now:**
  - In-app firmware flashing ("reset and restore" as an actual reflash flow).
    The reset button that *is* in scope only does a soft reboot (DTR/RTS
    pulse) — no firmware gets pushed. A full in-app flashing feature (an ESP32
    bootloader/flashing protocol built into the Kotlin app) was discussed and
    intentionally scoped out as a separate, much larger future project.
  - A raw binary ACK/NAK protocol byte. Deferred as a future teaching-curriculum
    topic once the user has more serial-protocol background — the fix below
    reuses `test_card.ino`'s existing human-readable print lines instead of
    introducing a new binary convention.

## The plan

### 1. Firmware — `workspace/sketches/test_card/test_card.ino`

`test_card.ino` already prints exactly what's needed for an ack, just at the
wrong time: `Serial.print("frame ok, ...")` currently fires *after*
`renderPayload()` (which includes the multi-second e-paper refresh), so today
it's not useful as a fast ack. The failure lines (`frame dropped: header read
timed out` / `bad length` / `payload read timed out` / `checksum mismatch`)
already fire early, right where each check fails — those don't need to move.

Change: move the `frame ok, ...` print to fire immediately after checksum
validation passes, *before* `renderPayload()`/the physical refresh. No new
protocol, no version field — same plain-text lines, just reordered so success
confirmation arrives within ~1-2s instead of waiting out the full panel
refresh.

### 2. Android — `UsbSerialTransport.kt`

`sendImage()` becomes retry-aware and reads the board's response:

- Write the frame, then read back a line with a timeout (~3s, now realistic
  since the ack fires pre-refresh), matching against `"frame ok"` or
  `"frame dropped: ..."`.
- **If the write itself fails** (USB link down, e.g. `rc=-1`): this is the
  link-flakiness case. Retry with a **full reconnect** — tear down the stale
  port, re-run device discovery + reopen (not just retry `write()` on the same
  handle, since a real detach/reattach hands out a new `UsbDevice`/connection
  that the old handle can't recover). **3 attempts total, 2 seconds apart.**
- **If the write succeeds but no response arrives** (timeout) or a
  `frame dropped: <reason>` line comes back: this is *not* a link problem, so
  no reconnect-retry. Surface it directly as a guidance message — the board
  may be running incompatible firmware (like `info_screen.ino`), or it
  explicitly rejected the frame for the stated reason.
- Every attempt, retry, timeout, and guidance message gets logged via the
  existing `log()` → `logEntries` mechanism, so the Diagnostics screen keeps
  full transparency (nothing happens silently).

New `resetBoard()`: pulses DTR/RTS (same technique as
`workspace/sketches/test_card/serial_sender.py`'s reset), logged like
everything else.

### 3. UI — `MainViewModel.kt` / `MainScreen.kt`

- New "Reset Board" button on `MainScreen`, enabled only when connected, calls
  `resetBoard()`.
- Replace the current *simulated* progress-bar timer (the `~1.3s write + ~4s
  cooldown` estimate, used today because the old fire-and-forget protocol gave
  no real completion signal) with **real, event-driven status text**, since
  the retry/ack logic above now provides genuine phase information:
  - `"Sending frame (attempt 1/3)..."`
  - `"USB error, reconnecting... (attempt 2/3)"`
  - `"Sent — waiting for board confirmation..."`
  - `"Confirmed: frame ok"`
  - `"Board didn't confirm — it may be running different firmware"`
  - `"Board rejected frame: <reason>"`
  - `"Failed after 3 attempts — check the USB connection"`

  This was an explicit user requirement: the UI must always show what's
  actually going on, not a simulated estimate.

## Files touched

- `workspace/sketches/test_card/test_card.ino`
- `app/src/main/java/com/example/crowpanelhub/UsbSerialTransport.kt`
- `app/src/main/java/com/example/crowpanelhub/MainViewModel.kt`
- `app/src/main/java/com/example/crowpanelhub/MainScreen.kt`

## Time estimate

- Firmware tweak + reflash + verify: ~20-30 min
- Android retry/reconnect/ack-read logic: ~1-1.5 hr
- Reset button wiring: ~20-30 min
- Hardware verification (deliberately trigger a dropout and confirm recovery;
  flash `info_screen.ino` and confirm the guidance message fires correctly;
  confirm Reset actually reboots the board): ~1-1.5 hr

**Total: ~3-4 hours** of coding + hands-on hardware testing.

## Note for whoever resumes this

Before starting, confirm what's actually flashed on the board right now (see
`CLAUDE.md`'s "confirm what's actually flashed" note) — don't assume it's
still `info_screen.ino` or still `test_card.ino` from this session.
