# Ideas

An idea pool for what the CrowPanel could do next — via the Android app or
standalone. Collected during a brainstorm session (2026-07-05), **not a
roadmap**: nothing here is a committed milestone, and the "natural
sequence" at the bottom is an observation about dependencies, not a plan.

Framing that shapes everything below: e-paper holds its image with **zero
power**, is readable in daylight, but a full refresh takes ~4 seconds and
visibly flashes. So the sweet spot is *information that changes
occasionally and is worth glancing at* — not animation, not anything
ticking every second. (Partial refresh bends this rule further than you'd
expect — see the games idea below — but glanceable-info remains the
natural fit.)

**Prior art**: the community has built a lot with ESP32 + e-ink, and it
validates this pool — weather/calendar dashboards dominate, and
multi-month battery life via deep sleep is proven in the wild (the
flagship weather display idles at ~14µA and runs 6–12 months per charge).
See `reference.md`'s "Community projects (prior art)" section for the
standout links. Across everything surveyed, e-ink ESP32 devices cluster
into four families: **dashboards** (dominant by far), **fleets** of many
small displays (shelf labels), **off-grid comms** (LoRa/Meshtastic), and
**wearables/novelty** — the ideas below now touch all four.

## Board hardware this project hasn't used yet

From the Elecrow wiki (see `reference.md` for the link), the board has
more than the panel and the USB port:

- **MENU button** (IO2) and **EXIT button** (IO1), free for custom use.
- **Rotary switch**, three-position: Down (IO4), Up (IO6), Config (IO5).
- **TF/SD card slot**, SPI-attached: MOSI IO40, MISO IO13, CLK IO39, CS IO10.
- **Battery connector** (SH1.0 2-pin, 3.7V LiPo) with an onboard charging
  circuit.
- **GPIO header**: IO3, IO9, IO15, IO17, IO19, IO21, IO8, IO14, IO16,
  IO18, IO20, IO38.
- Notably absent: no RTC, no buzzer, no LEDs, no onboard sensors — any of
  those would be external modules on the GPIO header. (No RTC is why the
  wire protocol carries a timestamp at all — see `CLAUDE.md`.)

## By learning curve

The same 30 ideas, tiered by how many *new* concepts each stacks on top of
what this repo already teaches (see `firmware-learning-path.md` for the
existing baseline). Independent of the Part 1/Part 2 hardware split below —
cheap-to-buy and easy-to-build are different axes.

- **Easy** — recombinations of existing pieces: 1 (desk badge),
  16 (photo sender), 17 (fridge note), 18 (composed-elsewhere dashboard),
  19 (QR code).
- **Medium** — one new on-board subsystem each: 2 (generative art frame),
  3 (NTP clock), 10 (menu/carousel), 12 (info screen), 13 (pomodoro
  timer), 14 (puzzle), 22 (SD carousel), 23 (album to SD), 24 (content
  playlists), 26 (battery + deep sleep), 27 (battery gauge),
  28 (sensor station).
- **Hard** — a whole new stack: 4 (weather/JSON), 5 (web server),
  6 (web config UI), 7 (OTA), 8 (Home Assistant), 9 (TRMNL client),
  11 (e-reader), 15 (partial-refresh games), 20 (BLE image transport),
  21 (notification mirror), 25 (data logger + charting),
  29 (LoRa mesh node), 30 (ESL access point).

## Part 1 — nothing to buy: the barebones kit is enough

Everything in this part runs on the board exactly as it ships — Wi-Fi and
BLE are built into the ESP32-S3, and the buttons/rotary are already on the
board.

### Standalone, no connectivity at all

1. **Desk badge / name tag** — draw once, unplug, the image stays
   forever. Pure e-paper party trick; small enough to join the
   `firmware-learning-path.md` sketches.
2. **Generative art frame** — the board draws its own algorithmic art on
   boot: mazes, cellular automata, plotter-style line work. Reuses
   `test_card.ino`'s existing Floyd-Steinberg dithering for grayscale
   effects; a new pattern every power-cycle with zero infrastructure.

### Standalone, needing Wi-Fi firmware first

3. **NTP clock/calendar** — the classic first Wi-Fi project. Also fixes a
   real limitation: today the board only knows the time when the phone
   tells it.
4. **Weather panel** — fetch a public API, parse JSON, render on-device.
5. **ESP32 web server** — the board serves a browser page; anyone on the
   network uploads an image directly. Makes the Android app optional.
6. **Web configuration UI** — the board serves its own settings page:
   choose what to display, set refresh schedules, build playlists (idea
   24). A meatier sibling of idea 5's upload-only page.
7. **OTA firmware updates** — reflash over Wi-Fi, no cable. Pairs well
   with the on-device info screen (idea 12) for showing what's running.
8. **Home Assistant / ESPHome status panel** — the panel renders
   home-automation state (who's home, sensor readings, alarms) driven by
   Home Assistant. A huge real-world category of e-ink builds.
9. **TRMNL-compatible client** — TRMNL is a commercial-but-open e-ink
   dashboard product built on exactly idea 18's architecture (server
   composes the image, board fetches and sleeps), with 375+ plugins and a
   self-hostable server (see `reference.md`). Implementing their fetch
   protocol would inherit that whole plugin ecosystem.

### Enabled by the buttons + rotary switch

10. **On-device menu/carousel** — rotary up/down cycles between stored
    screens; MENU/EXIT navigate. Teaches GPIO input + debouncing, absent
    from the repo so far.
11. **E-reader** — rotary page-turns, text from the SD card.
12. **On-device info screen** — press MENU, see the running sketch's name,
    build timestamp, free heap. Turns `CLAUDE.md`'s "confirm what's
    actually flashed" war story into a feature.
13. **Pomodoro / kitchen timer** — buttons start/reset; the panel shows
    remaining minutes, refreshed once a minute.
14. **Puzzle of the day** — sudoku/crossword on the panel; a button
    reveals the solution.
15. **Simple games** — the community has pushed partial refresh to ~60Hz
    on some panels (an entire e-ink Game Boy exists — see `reference.md`),
    so button-driven games well beyond "glanceable info" are provably
    possible. Even a modest version (2048, minesweeper via rotary) would
    be a fun partial-refresh stress test on this panel.

### Via the Android app (phone does the heavy lifting)

16. **Photo sender** — pick a gallery photo, resize + dither to 1-bit in
    the app, send. Zero firmware changes (`test_card.ino` draws whatever
    bitmap arrives over the wire protocol); completes the image-processing
    scope milestone 1 explicitly deferred.
17. **Fridge note / message board** — type text in the app, render, send.
    `TextBitmap.kt` is most of this already; it needs a text field instead
    of hardcoded demo text.
18. **Composed-elsewhere dashboard** — the app (or any other renderer)
    fetches weather/calendar/etc., composes one bitmap, sends it. All
    intelligence stays outside the board; firmware stays dumb. This is a
    community-proven architecture, not a beginner shortcut — several
    prior-art projects (see `reference.md`) run a home server that renders
    the image and let the board just fetch-and-display. Our wire protocol
    already supports any such renderer.
19. **QR code** — the removed former milestone 2; still valid someday as
    another app-side payload generator.
20. **BLE image transport** — send the same wire-protocol payload over
    Bluetooth LE instead of the USB cable: cable-free, no router involved.
    Notably the one transport from `CLAUDE.md`'s stated build order
    (USB → Wi-Fi → BLE) this pool didn't cover yet.
21. **Phone notification mirror** — the app forwards the phone's
    notifications over BLE; the panel shows the latest few, glanceable
    without picking up the phone.

## Part 2 — needs external hardware not included in the kit

The board has the *slots and connectors* for all of this, but the parts
themselves (card, battery, radio/sensor modules) ship separately. Each
subsection names what to buy.

### Enabled by the TF/SD card slot — needs: a TF/microSD card

22. **SD photo carousel** — load images onto the card; the board cycles
    them on a schedule or button press. A picture frame with zero
    infrastructure. Teaches SPI + filesystems.
23. **Album over the existing wire protocol** — the phone sends multiple
    frames, the board saves them to SD, the rotary flips through them.
    Bridges the app and standalone worlds with no protocol change.
24. **Content playlists** — generalize idea 22 from photos to
    heterogeneous screens: clock, then weather, then a photo, rotating on
    a schedule. Configured from SD, or via idea 6's web UI.
25. **Data logger + chart** — log sensor readings (GPIO header) to SD,
    render a chart.

### Enabled by the battery connector — needs: a 3.7V LiPo with SH1.0 plug

26. **Battery-powered anything** — deep sleep plus e-paper's zero hold
    power means weeks on a charge: meeting-room door sign, shelf label,
    plant tag, conference badge. Teaches ESP32 deep sleep + wake sources
    (timer, button) — the marquee ESP32 power trick. Solar is the proven
    next step up: a community solar e-ink watch ran 9 months without a
    charger (see `reference.md`).
27. **Battery gauge** — read pack voltage via ADC, show a battery icon in
    the label strip.

### Enabled by the GPIO header — needs: the module in question

28. **Sensor station** — a BME280 (temperature/humidity/pressure) or CO2
    sensor; the panel shows current values plus daily min/max.
29. **LoRa / Meshtastic mesh node** — an SX1262 LoRa module on the header
    turns the board into an off-grid text-message pager. E-ink is the
    *default* display in the Meshtastic world precisely because nodes run
    for days/weeks on battery (see `reference.md`) — this panel is
    oversized-luxurious by that community's standards.
30. **Electronic-shelf-label access point** — OpenEPaperLink (see
    `reference.md`) uses an ESP32 as an access point pushing content to
    fleets of cheap battery-powered e-ink price tags over an 802.15.4/BLE
    radio (external module needed). The CrowPanel as the hub of a house
    full of tiny displays, rather than being the display itself.

## Combinations (the standouts)

Both standouts lean on Part 2 purchases (battery; the first also a TF card):

- **Battery + SD + buttons** = a fully standalone picture frame/e-reader:
  no phone, no network — everything runs on the board itself.
- **Battery + Wi-Fi + deep sleep** = wake, fetch, render, sleep — a
  glanceable display that lives unplugged.

## What each theme actually takes

- **Interactivity (buttons/rotary)**: GPIO reading + debouncing, and —
  unavoidably — **partial refresh**, which this project has deliberately
  not used yet (see `CLAUDE.md`). A 4-second full-refresh flash per button
  press would feel broken; GxEPD2 supports partial refresh on this panel
  (`setPartialWindow()`), and managing its ghosting is the accompanying
  lesson. No purchases needed.
- **Storage (SD)**: an SD library over SPI plus a TF card. File format:
  skip BMP/PNG parsing entirely and store the wire protocol's exact
  15,000-byte packed payload — a file on the card is the same bytes as a
  frame on the wire, zero new parsing code.
- **Portability (battery)**: a 3.7V SH1.0 LiPo, the deep-sleep API, wake
  sources, and honest battery-life math as a teaching exercise.
- **Connectivity (Wi-Fi)**: two natural phases — hardcoded credentials in
  `config.h` first (simplest teaching step), captive-portal provisioning
  (e.g. WiFiManager) second. Then NTP, HTTP + JSON, mDNS, the web upload
  page, OTA.

## A natural sequence (an observation, not a commitment)

The themes stack by dependency and teaching order:

1. **Interactivity** — no purchases; unlocks partial refresh + GPIO input.
2. **Storage** — needs a TF card; with 1, becomes a button-driven gallery.
3. **Portability** — needs a LiPo; with 1+2, becomes a standalone picture
   frame that runs unplugged.
4. **Connectivity** — no purchases; with everything above, the full
   device: fetch content, serve uploads, OTA, real NTP time for the label.

Shopping list across all four: a TF card, a 3.7V SH1.0 LiPo, optionally a
BME280 — well under $20 total.

One recurring constraint: any idea that needs wall-clock time still
requires Wi-Fi/NTP, the phone's timestamp trick (the current approach —
see `CLAUDE.md`'s wire protocol notes), or an external RTC module on the
GPIO header. The board cannot know the time by itself.


## Check this
EPDiy E-Paper Driver - https://github.com/vroland/epdiy
