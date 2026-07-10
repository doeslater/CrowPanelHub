# Android app (`app/`)

The companion app that connects to the CrowPanel board over USB serial and
sends it a bitmap to render. See `CLAUDE.md` (repo root) for the
project-wide picture — current milestone, wire protocol, why USB serial
first. This file stays narrowly focused on the app's own code and flow.

## Files

| File | What it is |
| --- | --- |
| `CrowPanelHubApp.kt` | `@HiltAndroidApp` `Application` class — registers Hilt's DI container. |
| `MainActivity.kt` | Hosts the two-route `NavHost` (`"main"`/`"diagnostics"`) inside a `Scaffold`. |
| `MainViewModel.kt` | MVI-style state/actions for `MainScreen` — connect, send (real event-driven status text via `sendImage`'s `onStatus` callback), and reset. See the flowchart below. |
| `MainScreen.kt` | Transport picker, Connect button, Send Text/Send Checkerboard/Reset Board buttons, status row. |
| `UsbSerialTransport.kt` | Talks to the board via `usb-serial-for-android` — permission flow, opening the port, writing frames and reading back acks, reconnecting on a write failure, resetting the board, and the Diagnostics log. |
| `DiagnosticsViewModel.kt` / `DiagnosticsScreen.kt` | Read-only screen mirroring `UsbSerialTransport`'s connection state and event log. |
| `WireFrame.kt` | Builds the wire-protocol frame (magic byte, length, timestamp, checksum) around a payload. |
| `TextBitmap.kt` / `Checkerboard.kt` | The two payload generators wired to the Send buttons. |
| `ui/theme/{Color,Theme,Type}.kt` | Default Material 3 theme, unmodified template. |

## Code walkthrough

![Flowchart of the Android app's Connect and Send flows: tapping Connect finds a USB serial driver, requests permission if needed, and opens the port, bailing out to a shared ConnectFailed/PermissionDenied box on any failure and letting the user retry; once Connected, tapping Send Text or Send Checkerboard builds a wire-protocol frame and writes it to the port with a simulated progress bar, bailing out to a Send-failed box on a write error, and looping back so another send can be tried.](android-flowchart.png)

**This diagram predates the USB-reliability fix (`docs/usb-reliability-fix-plan.md`)
and still shows the old simulated-progress-bar, no-retry, no-Reset-Board Send flow —
regenerate `android-flowchart.png` via `generate_android_flowchart.py` before
trusting the picture over the prose below.**

Read this alongside `MainViewModel.kt`/`UsbSerialTransport.kt` — the inline
comments there go into more depth on individual calls (why `FLAG_MUTABLE`
and `RECEIVER_NOT_EXPORTED` both matter for the permission dialog, why a
write failure and an ack timeout/rejection are retried differently, etc.)
than the diagram does.

- **Connect** (`UsbSerialTransport.connect()`): finds a USB serial driver
  via `UsbSerialProber`, requests permission if the app doesn't already
  have it (Android's permission dialog is async, bridged into a suspend
  call via a `BroadcastReceiver`), then opens the port at 115200 baud/8N1.
  Any failure along the way updates `connectionState` to `ConnectFailed`
  or `PermissionDenied` — there's no automatic retry, the user just taps
  Connect again.
- **Send** (`MainViewModel`'s `send()` + `UsbSerialTransport.sendImage()`):
  builds a payload (`TextBitmap.generate()` or `Checkerboard.generate()`),
  wraps it in a wire-protocol frame via `WireFrame.build()`, writes it to
  the open port, then reads back `test_card.ino`'s ack line (`readAckLine()`,
  3s timeout). A write failure retries via a full reconnect (rediscover +
  reopen the port, 3 attempts, 2s apart); a timeout or an explicit
  `"frame dropped"` rejection is *not* retried and is surfaced as guidance
  instead (e.g. wrong firmware flashed). `onStatus` reports each phase as
  real, event-driven status text — see `docs/usb-reliability-fix-plan.md`
  for the full design.
- **Reset Board** (`MainViewModel`'s `resetBoard()` action +
  `UsbSerialTransport.resetBoard()`): pulses the RTS line for 100ms, the
  same electrical event as pressing the board's physical reset button —
  reboots whatever firmware is currently flashed, from the top. Doesn't
  touch flash memory, so it can't reflash or lose firmware (see
  `CLAUDE.md`'s Open threads for the deferred idea of actual in-app
  reflashing).
- **Diagnostics** isn't its own branch in the diagram — it's a passive
  observer that mirrors `connectionState`/`logEntries` at every step of
  both flows above, with nothing to trigger on that screen itself.

## Regenerating the flowchart

No diagramming tool (`graphviz`, `mermaid-cli`) is installed on the
reference dev machine, and installing one needs a system package. So
`generate_android_flowchart.py` hand-draws the diagram with Pillow instead
— the same approach `workspace/sketches/test_card/generate_flowchart.py` uses.
Edit the script and rerun it if `MainViewModel.kt`/`UsbSerialTransport.kt`'s
logic changes:

```bash
python3 docs/generate_android_flowchart.py   # from the repo root, like the commands above
```
