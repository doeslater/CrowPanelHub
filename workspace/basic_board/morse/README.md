# ESP32 WiFi Morse Blinker

Blinks a message out on the onboard LED (GPIO2) in Morse code, on repeat,
starting with the built-in default **Hello World**. It also hosts a small web
page on your local network — type a new message there and the LED switches to
blinking that instead.

(This folder merges the earlier `morse-code/` — standalone looping blinker —
and `morse-wifi-server/` — blink-on-request server — into one sketch.)

## Setup

1. Edit `wifi_config.h` in this folder and replace the placeholders with your
   real network credentials:

   ```c
   #define WIFI_SSID "your-network-name"
   #define WIFI_PASSWORD "your-network-password"
   ```

   `wifi_config.h` holds your password in plain text — keep it out of
   version control (it's gitignored here). `wifi_config.h.example` is the
   template to share instead.

2. Build and upload, from this folder:

   ```sh
   ./run_morse.sh upload
   ```

3. Watch the serial output to get the board's IP address once it connects:

   ```sh
   ./run_morse.sh monitor
   ```

   It prints `Connected. Open http://<ip-address>` once WiFi comes up
   (blinking starts before that, without waiting), then a `[blink]` line at
   the start of each blink pass, a `[heartbeat]` line every 5s, and a
   `[request]` line for every page load or submission, so there's always
   live output to watch — the heartbeat shows `wifi=disconnected` while it's
   still trying to connect. (This uses a raw serial read rather than
   `arduino-cli monitor` — the latter produced garbled output against this
   board.) Ctrl-C to exit.

   Run `./run_morse.sh` with no arguments for a menu, or `check` for a quick
   10-second sanity read instead of a live session.

## Usage

The LED starts blinking Hello World immediately at power-on — no network or
browser needed; WiFi connects in the background and the web server comes up
whenever it lands. To change the message, open the printed address on the same
network, type something, and hit "Blink it." The new message interrupts the
current blink pass at the next symbol boundary and loops from then on.
Letter case doesn't matter (Morse has none — messages display as typed and
blink the same either way). Unsupported characters (anything outside
letters, digits, and spaces) are silently skipped; an empty submission is
ignored.

## Notes

- The server stays responsive while blinking: every Morse delay runs through
  `serverDelay()`, which keeps servicing HTTP requests in small slices, so
  page loads answer within milliseconds even mid-message. (The old
  morse-wifi-server blocked until a message finished; that limitation is
  gone.)
- The message only lives in RAM — a reset goes back to the HELLO WORLD
  default.
- Default FQBN/port here assume the classic ESP32 DevKitC/WROOM-32 board on
  `/dev/ttyUSB0` — adjust if yours differs (see `config.json` in this
  folder).
