# Command-line tools used on this project

This project is explicitly a learning exercise (see `CLAUDE.md`), so this file documents the
command-line tools actually run while working in this repo — what each one is for, how to install
it, and the specific commands used here — so you can run them yourself instead of only watching
them happen. Tools are grouped into **in use now** and **planned** (installed later, once
firmware/USB work starts).

Everything below assumes a Linux shell and the repo root (`~/rnd/CrowPanelHub` on this machine) as
the working directory unless noted.

## In use now

### git

Version control for the whole repo (Android app + firmware + docs together).

Already installed on virtually every dev machine; if not: `sudo dnf install git` (Fedora) /
`sudo apt install git` (Debian/Ubuntu).

Commands actually used in this project:

```bash
git status                      # see what's changed/untracked before doing anything else
git log --oneline -10           # recent commit history
git diff                        # unstaged changes
git diff --staged               # staged changes not yet committed
git add app/src/...             # stage specific files (never `git add -A`/`git add .`)
git commit -m "..."             # commit staged changes
git branch                      # list branches, confirm current one
```

This repo has no `LICENSE` file and no CI — see `CLAUDE.md` for why.

### Gradle wrapper (`./gradlew`)

Builds, tests, and installs the Android app (`app/`). Always invoke via the wrapper script
(`./gradlew`), never a system-wide `gradle` — the wrapper pins the exact Gradle version this
project needs (see `gradle/wrapper/gradle-wrapper.properties`), so nothing needs to be installed
separately.

```bash
./gradlew assembleDebug                              # build a debug APK
./gradlew testDebugUnitTest                          # run JVM unit tests (app/src/test)
./gradlew testDebugUnitTest --tests "com.example.crowpanelhub.ExampleUnitTest"   # a single test
./gradlew connectedDebugAndroidTest                  # instrumented tests (needs a device/emulator)
./gradlew lint                                       # Android Lint
./gradlew installDebug                               # install the debug build on a connected device/emulator
```

First run of any `./gradlew` command downloads the pinned Gradle distribution — expect it to be
slow once, fast after.

### `android` CLI

A single command-line tool (already installed at `~/.local/bin/android`, i.e. on `PATH`) that
wraps the Android SDK toolchain — SDK package management, emulator lifecycle, deploying an APK,
and screen capture — so you don't need to remember separate `sdkmanager`/`avdmanager`/`adb`
invocations for the common cases. `adb` itself is still installed underneath it (see below) for
anything the wrapper doesn't cover.

```bash
android info                    # show SDK location/version (SDK: ~/Android/Sdk on this machine)
android sdk list                # installed + available SDK packages (platforms, build-tools, system images)
android sdk install <package>   # install an SDK package, e.g. "platforms;android-35"

android emulator list           # list virtual devices (this machine has Pixel_9a, Nexus5_API27)
android emulator create ...     # create a new virtual device
android emulator start <name>   # boot an emulator and wait until it's ready
android emulator stop <name>    # shut it down

android run --apks app/build/outputs/apk/debug/app-debug.apk --activity .MainActivity   # install + launch
android screen capture -o screenshot.png   # save the current device/emulator screen as a PNG
android screen resolve          # visually target UI elements on screen (for scripted interaction)
```

Run `android <command> --help` for the full flag list of any subcommand. Two gotchas hit in
practice:
- `android screen capture` with no `-o` writes `screenshot.png` into the current directory —
  pass `-o <path>` explicitly, or it'll land in the repo root and show up as an untracked file.
- `android screen capture` has no device/serial flag: with more than one device or emulator
  online it just errors ("Multiple devices are currently online"). Drop to plain adb instead
  (see below) when you need to target one specific device.

### adb (Android Debug Bridge)

The lower-level tool the `android` CLI builds on. Installed as part of the SDK's
`platform-tools` package, at `~/Android/Sdk/platform-tools/adb` — not on `PATH` by default, so
add the directory once (persisted in `~/.bashrc` on this machine):

```bash
export PATH="$HOME/Android/Sdk/platform-tools:$PATH"   # add to ~/.bashrc to persist
```

Reach for `adb` directly when you need something the `android` CLI doesn't expose — in particular,
targeting one specific device (via `-s <serial>`, from `adb devices`) when more than one is
online, which `android`'s subcommands generally can't do:

```bash
adb devices -l                   # list connected devices/running emulators, with details
adb logcat                       # stream device log output (filter with e.g. `adb logcat *:E`)
adb install app/build/outputs/apk/debug/app-debug.apk
adb shell pm list packages | grep crowpanelhub
adb uninstall com.example.crowpanelhub

# with a physical device *and* an emulator both online, target one explicitly:
adb -s <serial> shell am start -n com.example.crowpanelhub/.MainActivity   # launch an installed app
adb -s <serial> exec-out screencap -p > device-screenshot.png             # screenshot that one device
```

`./gradlew installDebug` (see above) installs to *every* connected/authorized device at once —
no need to loop or pick a target for that one.

Relevant to this project's own roadmap: once USB serial transport work starts, `adb` and the
`usb-serial-for-android` library are unrelated (adb talks to the *phone* over USB; the app will
separately talk to the *ESP32* over USB) — don't conflate the two USB connections.

## In use now (continued)

### `arduino-cli`

The command-line equivalent of the Arduino IDE GUI — installed and used here to compile
`display_text/` and to compile/upload/test `receive_image/` without ever opening the GUI,
confirming the board config in `docs/arduino-ide-setup.md` translates directly into `arduino-cli`
flags.

```bash
# install into ~/.local/bin (already on PATH — same dir as the `android` CLI)
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=$HOME/.local/bin sh
arduino-cli version                           # confirm install

arduino-cli core update-index                 # refresh the default (Arduino-maintained) board index

# ESP32 boards (esp32:esp32:*) live in Espressif's own index, not the default one —
# register it once, then re-run update-index so it's picked up:
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index

arduino-cli core install esp32:esp32          # ESP32 board package (see docs/reference.md) — ~1GB, several minutes
arduino-cli lib install GxEPD2                # e-paper driver library used by display_text/ (pulls in Adafruit GFX + BusIO)

arduino-cli board listall | grep -i esp32s3   # find the FQBN for "ESP32S3 Dev Module" -> esp32:esp32:esp32s3
arduino-cli board details -b esp32:esp32:esp32s3   # list valid values for each --fqbn board-option (USBMode, PSRAM, etc.)

# compile, matching the board options recorded in docs/arduino-ide-setup.md (works for either sketch):
arduino-cli compile --fqbn "esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=default,PSRAM=opi,PartitionScheme=default" display_text/
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32s3 receive_image/   # actually flashed to real hardware, see below
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

Confirmed working on this machine: the compile command above succeeds against the real
`display_text/display_text.ino` (458,538 bytes / 34% program storage, 39,188 bytes / 11% dynamic
memory). The four `--fqbn` board-option values (`hwcdc`, `default`, `opi`, `default`) were checked
against `arduino-cli board details` first — worth doing since `arduino-cli compile` does reject an
invalid option value outright (`Error during build: invalid value '...' for option '...'`), so a
typo there fails loudly rather than silently compiling with the wrong setting.

`upload` has also been exercised for real: `receive_image/` (the milestone-1 firmware — see
`CLAUDE.md`) was compiled and flashed to an actual CrowPanel board on `/dev/ttyUSB0` with the same
`--fqbn` string as above, and confirmed working via the serial round-trip described below.

### Serial port permissions (Linux)

Uploading/monitoring over `/dev/ttyUSB0` needs the device file to be readable/writable by your
user, which by default it usually isn't:

```bash
ls -l /dev/ttyUSB0                        # e.g. crw-rw---- root dialout -- your user probably isn't in "dialout"
sudo usermod -a -G dialout $USER          # one-time fix, persists across reboots
```

Group membership changes only apply to *new* login sessions, not the shell you ran `usermod` from.
Rather than fully logging out, a subshell with the group applied immediately works too:

```bash
sg dialout -c 'arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32s3 receive_image/'
```

### Python (serial control, no extra install)

There's no Android sender yet to actually produce a wire-protocol frame, so a small Python script
(`receive_image/send_test_frame.py`) stands in for it during firmware bring-up — it resets the
board and sends a hand-built frame matching the protocol in `CLAUDE.md`. `pip`/`pyserial` weren't
available on this machine (`python3 -m pip` → `No module named pip`, and installing it needs
`sudo`/network access we didn't want to assume), so the script talks to the serial port using only
the Python standard library:

```python
import fcntl, os, termios
fd = os.open("/dev/ttyUSB0", os.O_RDWR | os.O_NOCTTY)   # open the port
termios.tcsetattr(fd, termios.TCSANOW, [...])            # configure raw mode, 115200 baud
fcntl.ioctl(fd, 0x5416, struct.pack("I", 0x004))         # TIOCMBIS + TIOCM_RTS: pulse RTS to reset the board
os.write(fd, frame_bytes)                                 # send a frame
os.read(fd, 4096)                                         # read the board's Serial.println() responses
```

Run it directly once the board is flashed and plugged in:

```bash
python3 receive_image/send_test_frame.py
```

A useful adjacent trick: `esptool` (bundled with the ESP32 core, not on `PATH` by itself) can
reset a board into its already-flashed app **without reflashing**, handy for re-triggering a test
without waiting through another upload:

```bash
~/.arduino15/packages/esp32/tools/esptool_py/5.3.0/esptool --chip esp32s3 --port /dev/ttyUSB0 run
```

`receive_image/send_text.py` builds on the same idea but sends rendered text instead of a
checkerboard — proof that `receive_image.ino` doesn't need to change at all to display something
different, since it just draws whatever 15,000-byte bitmap it's handed. Unlike
`send_test_frame.py`, this one isn't stdlib-only: it uses Pillow (`PIL`) to rasterize text with
real font metrics, since hand-rolling a bitmap font wasn't worth it for a test script. Pillow
happened to already be installed on this machine; where it isn't, `pip install pillow` (or your
distro's `python3-pillow` package) is needed first.

```bash
python3 receive_image/send_text.py "Hello World" "from Python" "over USB serial"
python3 receive_image/send_text.py                              # uses built-in default lines
```

## Planned (not installed yet)

### PlatformIO CLI (`pio`)

The planned long-term firmware toolchain (see `CLAUDE.md`) — chosen over the Arduino IDE because
it's scriptable and version-pins dependencies via `platformio.ini`, mirroring
`gradle/libs.versions.toml` on the Android side. Not installed on this machine, and no
`platformio.ini` exists in this repo yet (migrating `display_text/`, or writing the milestone-1
sketch directly as a PlatformIO project, is still pending — see `CLAUDE.md` Open threads).

```bash
# install (see https://docs.platformio.org/en/latest/core/installation/methods/index.html)
python3 -m pip install --user platformio

pio run                          # build the firmware project (needs platformio.ini)
pio run -t upload                # build and flash over USB
pio device monitor -b 115200     # open a serial monitor at the project baud rate
pio device list                  # list connected serial ports (find the ESP32's /dev/ttyUSB*)
```

## See also

- `docs/reference.md` — links for the hardware, GxEPD2/SSD1683, and every library/toolchain named above.
- `docs/arduino-ide-setup.md` — the known-working Arduino IDE board configuration for this board.
- `CLAUDE.md` — project goals, current milestone, and the Commands section this file expands on.
