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
| `MainViewModel.kt` | MVI-style state/actions for `MainScreen` — connect, send, and the simulated send-progress bar. See the flowchart below. |
| `MainScreen.kt` | Transport picker, Connect button, Send Text/Send Checkerboard buttons, status row. |
| `UsbSerialTransport.kt` | Talks to the board via `usb-serial-for-android` — permission flow, opening the port, writing frames, and the Diagnostics log. |
| `DiagnosticsViewModel.kt` / `DiagnosticsScreen.kt` | Read-only screen mirroring `UsbSerialTransport`'s connection state and event log. |
| `WireFrame.kt` | Builds the wire-protocol frame (magic byte, length, timestamp, checksum) around a payload. |
| `TextBitmap.kt` / `Checkerboard.kt` | The two payload generators wired to the Send buttons. |
| `ui/theme/{Color,Theme,Type}.kt` | Default Material 3 theme, unmodified template. |

## Code walkthrough

![Flowchart of the Android app's Connect and Send flows: tapping Connect finds a USB serial driver, requests permission if needed, and opens the port, bailing out to a shared ConnectFailed/PermissionDenied box on any failure and letting the user retry; once Connected, tapping Send Text or Send Checkerboard builds a wire-protocol frame and writes it to the port with a simulated progress bar, bailing out to a Send-failed box on a write error, and looping back so another send can be tried.](android-flowchart.png)

Read this alongside `MainViewModel.kt`/`UsbSerialTransport.kt` — the inline
comments there go into more depth on individual calls (why `FLAG_MUTABLE`
and `RECEIVER_NOT_EXPORTED` both matter for the permission dialog, why the
send progress bar is simulated rather than real, etc.) than the diagram
does.

- **Connect** (`UsbSerialTransport.connect()`): finds a USB serial driver
  via `UsbSerialProber`, requests permission if the app doesn't already
  have it (Android's permission dialog is async, bridged into a suspend
  call via a `BroadcastReceiver`), then opens the port at 115200 baud/8N1.
  Any failure along the way updates `connectionState` to `ConnectFailed`
  or `PermissionDenied` — there's no automatic retry, the user just taps
  Connect again.
- **Send** (`MainViewModel`'s `send()` + `UsbSerialTransport.sendImage()`):
  builds a payload (`TextBitmap.generate()` or `Checkerboard.generate()`),
  wraps it in a wire-protocol frame via `WireFrame.build()`, and writes it
  to the open port. The protocol is fire-and-forget — the ESP32 never
  confirms it actually finished drawing — so the progress bar just ticks
  across an estimated write-time-plus-panel-refresh window rather than
  reporting real completion.
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
