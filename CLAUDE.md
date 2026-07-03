# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

"CrowPanelHub" is an exploration project for the **CrowPanel ESP32 4.2" e-paper HMI display** (see Sources below). The explicit goal is to understand the hardware and **help other developers dive into the ESP32 world** — this is a teaching/learning project, not a polished product. It is intended to be published as a **public repo under the MIT license** (not done yet — no `LICENSE` file, and the repo is not yet even a git repository as of this writing). The Android app (package `com.example.crowpanelhub`) is meant to become the companion app that sends images to the panel.

**Current state is a bare scaffold, on both sides — read this before trusting anything else in this file about "current" architecture:**
- `app/` is an unmodified Android Studio "Empty Activity" (Compose) template: one `MainActivity`, one `Greeting` composable, default theme files. No navigation, no ViewModel, no repository, no hardware/transport integration exists yet.
- `display_text/` is a standalone Arduino `.ino` sketch (not part of the Android build, not using PlatformIO) adapted from a third-party Elecrow example. It renders a fixed set of hardcoded text rows to the panel on boot. It is **not** the milestone-1 image-transfer firmware described below — no serial receive loop, no USB protocol, nothing wired to the phone.

Do not assume any USB/BLE/Wi-Fi code, navigation, ViewModel, or repository code exists in this repo until it's actually been added — check the current files before referencing them, since most of this document (outside this section and Sources) describes the *planned* design, not what's built.

See `MEMORY.md` at the repo root for the narrative/rationale behind earlier planning decisions. Note it was written before the app and firmware were reset to their current bare-scaffold state, so treat its description of "current" code the same way as this file's Planned sections — as intent, not fact.

## Hardware target

- **Board**: CrowPanel ESP32 4.2" E-Paper HMI Display, SKU DIE07300S (Elecrow), ESP32-S3-WROOM-1-N8R8.
- **Panel**: 400×300, black/white only, driven by SPI, SSD1683 controller.
- **Firmware ecosystem**: Arduino-based; Elecrow ships demo/example sketches, and the GxEPD2 library is the common driver for this panel (see Sources). The existing `display_text/` sketch uses `GxEPD2_BW<GxEPD2_420_GYE042A87, ...>` from that library.
- **Firmware lives in this repo**: ESP32 firmware is authored as a subproject alongside `app/`, as a sibling directory at the repo root (e.g. `display_text/`), not nested inside the Android module.

## Current milestone

**Milestone 1**: send a hardcoded checkerboard test pattern from the phone to the ESP32 over USB serial and see it render on the panel, using a full (not partial) e-paper refresh. The pattern is generated programmatically in Kotlin (a small loop building packed 1-bit bytes) — no bundled binary asset — so the code is readable/teachable and so image processing isn't a variable while proving out the USB→panel pipeline. This is fire-and-forget: the ESP32 does not send anything back, and Android considers the send successful once the bytes are written to the serial port. Visual confirmation is by looking at the panel during bring-up. **Not started** — neither the Android send path nor a serial-receiving firmware sketch exists yet.

**Milestone 2** (after milestone 1 works): swap the checkerboard for a QR code (via a library such as ZXing) that links to information about the project/how it was built — the panel becomes something a person could scan and learn from. The QR's target URL is not yet decided.

USB serial was chosen to start (over Wi-Fi/BLE) because it needs no pairing/provisioning UI, giving the tightest feedback loop while the ESP32-side rendering path is still being proven out.

## Planned architecture (not yet implemented)

- **Transport abstraction**: a thin interface (e.g. `suspend fun sendImage(bytes: ByteArray): Result<Unit>` + `connectionState: Flow<ConnectionState>`) is meant to be introduced when transport work starts, so the ViewModel/UI layer doesn't couple to one transport's connection semantics (BLE's async connect vs. Wi-Fi's stateless HTTP vs. USB serial). Keep it minimal — just enough to add Wi-Fi/BLE later without reworking callers, not a speculative plugin system.
- **Build order**: USB serial first, then Wi-Fi, then BLE (all three are intended to eventually be supported).
- **Single active transport**: only one transport is ever connected at a time. The user explicitly picks which one (USB/Wi-Fi/BLE) — no simultaneous multi-transport support.
- **Android USB serial library**: use the community `usb-serial-for-android` (mik3y) library rather than raw `UsbManager`/CDC-ACM handling — it probes for and supports the common bridge chip families (CP210x, CH340/CH9102, FTDI, PL2303) plus native CDC-ACM, so it works regardless of which USB-to-serial implementation the CrowPanel board turns out to use (undetermined from Elecrow's docs/schematic — the port is labeled "UART0" not "USB", suggesting a discrete bridge chip, but this isn't confirmed and doesn't need to be to pick the library).
- **Connect UX**: a manual "Connect" button, not auto-launch via a `USB_DEVICE_ATTACHED` intent-filter — no hardcoded VID/PID, and the permission flow stays visible in the UI rather than manifest magic.
- **Screen layout**: `MainScreen` hosts the transport picker, the Connect button, and the Send (test pattern) button all together — one screen for the one real workflow. The Diagnostics screen is separate, reachable from Settings, and is **read-only** (connection state/history, last error) — it is not where a connection is initiated.
- **Diagnostics screen**: a permanent, user-facing screen (not dev-only / not gated behind `BuildConfig`) surfacing live connection state and transport activity.
- **Wire protocol** (phone → ESP32 only, one-directional): a minimal length-prefixed binary frame — `[1-byte magic][4-byte little-endian payload length][payload: packed 1-bit bitmap, 400×300 = 15,000 bytes][1-byte checksum]`. Binary, not text/ASCII, since the payload is already binary.
- **Baud rate**: 115200 — the universal ESP32/Arduino default (also what the existing `display_text/` sketch uses for its `Serial` logging), and more than fast enough (~1.3s per frame) next to the multi-second e-paper refresh itself.
- **Firmware refresh mode**: full refresh only for milestone 1 (`epd.display()`), not partial refresh — simplest, matches every GxEPD2 example (including `display_text/`, which calls `epd.init(..., partial = false)`), and ghosting/speed concerns from partial refresh aren't relevant when sending one deliberate frame at a time.
- **Firmware toolchain**: PlatformIO (CLI-first: `pio run`, `pio run -t upload`, `pio device monitor`), not the Arduino IDE — chosen because it's scriptable from the terminal and gives version-pinned library deps via `platformio.ini`, mirroring `libs.versions.toml` on the Android side. **Not installed on this machine yet**, and the current `display_text/` sketch is a plain `.ino`/`config.h` pair with no `platformio.ini` — migrating it (or the milestone-1 sketch) to PlatformIO is still pending.
- Writing up findings/notes from this exploration for other developers is an out-of-repo activity (docs/blog-style), not an in-app feature — don't build a "share log" or similar mechanism unless explicitly asked again.

## Open threads (not yet decided)

- Copyright name/entity for the `LICENSE` file (MIT, once added).
- Target URL the milestone-2 QR code should encode.
- Error-state UX on Main/Diagnostics when USB permission is denied or the device disconnects mid-send.
- When to `git init` and create the actual public GitHub repo (repo is not a git repository yet).
- Migrating firmware to a PlatformIO project structure (`platformio.ini` + `src/`) instead of a bare `.ino`.

## Commands

All commands run from the repo root via the Gradle wrapper.

- Build debug APK: `./gradlew assembleDebug`
- Run local unit tests (JVM, in `app/src/test`): `./gradlew testDebugUnitTest`
- Run a single unit test: `./gradlew testDebugUnitTest --tests "com.example.crowpanelhub.ExampleUnitTest"`
- Run instrumented/UI tests (in `app/src/androidTest`, requires a connected device/emulator): `./gradlew connectedDebugAndroidTest`
- Run Android Lint: `./gradlew lint`
- Install debug build on a connected device: `./gradlew installDebug`

There is no ktlint/detekt/spotless config and no CI workflow in this repo — lint/test enforcement is whatever you invoke manually via Gradle. Both `app/src/test/.../ExampleUnitTest.kt` and `app/src/androidTest/.../ExampleInstrumentedTest.kt` are still the unmodified Android Studio template tests.

There is no firmware build tooling yet (no PlatformIO install, no `platformio.ini`); `display_text/display_text.ino` is currently only buildable/flashable via the Arduino IDE (with the GxEPD2 library and its font headers installed). See `docs/arduino-ide-setup.md` for a known-working board configuration (from a successful upload, transcribed from a screenshot).

## Architecture (current code)

- **`app/`**: the default Android Studio "Empty Activity" Compose template, unmodified beyond the package rename to `com.example.crowpanelhub`.
  - `MainActivity.kt`: a single `ComponentActivity` that calls `setContent` with `CrowPanelHubTheme { Scaffold { Greeting(...) } }`. `Greeting` just renders `Text("Hello $name!")`. No navigation, no ViewModel, no state, no data layer.
  - `ui/theme/{Color,Theme,Type}.kt`: default Material 3 theme (`CrowPanelHubTheme`), unmodified template colors/typography, dynamic color enabled on API 31+.
  - No `data/`, `viewmodel/`, or `navigation` code exists — don't reference `DataRepository`, `MainScreenViewModel`, or any `NavKey`/back-stack setup; none of it is in the tree.
- **`display_text/`** (repo root, sibling of `app/`): a standalone Arduino sketch, not wired into the Gradle build. Placed here deliberately as a **reference example** (GxEPD2 init/pin-mapping/power-cycle patterns) — do not modify or delete it, and do not treat it as a spike to evolve into the milestone-1 firmware; write new sketch files for that instead, consulting this one for patterns.
  - `display_text.ino`: defines a `DisplaySettings`/`Row` model (up to 8 text rows, per-row font size, optional border/color-inversion), a `GxEPD2_BW<GxEPD2_420_GYE042A87, ...>` display instance on pins `PWR=7 BUSY=48 RES=47 DC=46 CS=45`, and an `updateDisplay()` that powers the panel, draws the configured rows, calls `epd.display()`, then hibernates and powers down. `setup()` hardcodes 5 rows of demo text ("Hello World" / "Second line" / ...) and calls `updateDisplay()` once; `loop()` just delays — no serial input handling, no dynamic content.
  - `config.h`: compile-time constants (display size 400×300, margin, baud rate, font sizes, row heights).

## Build configuration notes

- Kotlin 2.2.10, AGP 9.2.1, Gradle 9.4.1 (see `gradle/wrapper/gradle-wrapper.properties`).
- compileSdk/targetSdk 37, minSdk 24. `sourceCompatibility`/`targetCompatibility` are JavaVersion 11 (not 17).
- Compose compiler is applied via the `org.jetbrains.kotlin.plugin.compose` plugin (Kotlin 2.x style), not the old `composeOptions.kotlinCompilerExtensionVersion`.
- No `kotlinx.serialization` plugin is applied anywhere in the project yet (root `build.gradle.kts` only applies `android.application` and `kotlin.compose`, both with `apply false`).
- Dependency versions are centralized in `gradle/libs.versions.toml` (version catalog) — add new dependencies there rather than hardcoding coordinates in `app/build.gradle.kts`.

## Sources

See `docs/reference.md` for all external links (hardware/product pages, GxEPD2, Arduino-ESP32 core, PlatformIO, and the planned `usb-serial-for-android`/ZXing Android libraries) — kept in one file so they don't drift out of sync across `README.md`/`CLAUDE.md`/`MEMORY.md`. Add new links there first.
