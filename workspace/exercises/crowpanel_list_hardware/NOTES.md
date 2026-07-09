
### Timesmap: 13:07

## 2026-07-09 — CrowPanel's programming port is CH340, not native USB
This board is ESP32-S3 with a native USB peripheral, but its programming
port is wired through an onboard CH340 bridge chip instead (confirmed by
`doeslater/CrowPanelHub`'s own CLAUDE.md). That means it shows up on the USB
bus with the exact same generic CH340 vendor/product ID as any other
CH340-based board (including the classic ESP32 board used for morse-code in
this sandbox) — `lsusb`/`arduino-cli board list` can't tell them apart.
Swapping which physical board is plugged into `/dev/ttyUSB0` is invisible to
USB-bus scanning. The fix was to query the chip itself over the serial port
(`esptool --port /dev/ttyUSB0 chip-id`), which reports real silicon info
(ESP32 vs ESP32-S3, PSRAM size, MAC) and definitively confirms which board is
actually connected, regardless of USB descriptor collisions.

## 2026-07-09 — WiFi.macAddress() read all zeros without WiFi initialized
`WiFi.macAddress()` needs the WiFi driver initialized (e.g. via
`WiFi.mode(WIFI_STA)` or `WiFi.begin()`) before it returns a real value —
called cold, it read back `00:00:00:00:00:00`. Since this sketch has no
actual need for WiFi, the fix was `ESP.getEfuseMac()` instead, which reads
the MAC straight from eFuse without touching the radio at all. Also dropped
the `WiFi.h` include entirely, which shrank the compiled binary from 28% to
22% of flash — pulling in the WiFi stack for one function call that didn't
need it was costing real space.

## Sources

References consulted while building `crowpanel_list_hardware.ino`.

### Board identity & pinout

- [CrowPanel ESP32 E-paper HMI 4.2-inch Display — Elecrow Wiki](https://www.elecrow.com/wiki/CrowPanel_ESP32_E-paper_4.2-inch_HMI_Display.html) — MCU (ESP32-S3-WROOM-1-N8R8), button pins, SD card SPI pins, general-purpose header pin list.
- [CrowPanel ESP32 E-Paper 4.2-inch Arduino Tutorial — Elecrow Wiki](https://www.elecrow.com/wiki/CrowPanel_ESP32_E-Paper_4.2-inch_Arduino_Tutorial.html) — recommended Arduino IDE board settings (ESP32S3 Dev Module, PSRAM=OPI, Partition Scheme).
- [CrowPanel ESP32-S3 4.2" E-paper HMI Display: pinout, datasheet and specs — Mischianti](https://mischianti.org/crowpanel-esp32-s3-4-2-e-paper-hmi-display-high-resolution-pinout-datasheet-and-specs/) — full GPIO table (e-paper SPI incl. SCK/MOSI, SD card, buttons, header), used to cross-check the Elecrow wiki.
- [github.com/doeslater/CrowPanelHub](https://github.com/doeslater/CrowPanelHub) — the user's own hardware-verified project for this exact board; its `display_text.ino`/`CLAUDE.md` confirmed the e-paper CS/DC/RST/BUSY/PWR pin assignments and that the board's programming port is a CH340 bridge chip (not native USB), which is what `crowpanel_list_hardware.ino`'s pin-map pages and code comments are checked against.

### Battery voltage (not implemented — dead end)

- [ESP32-WROOM CrowPanel 2.4inch — Battery external connector — Arduino Forum](https://forum.arduino.cc/t/esp32-wroom-crowpanel-2-4inch-battery-external-connector/1289114) — confirms the VBAT/battery-ADC line isn't actually wired on this board revision; monitoring it needs a hardware mod (external resistor divider or soldering), so no battery-voltage page was added.

### ESP32-S3-general facts (not board-specific)

- ADC1/ADC2 channel-to-GPIO mapping, RTC-GPIO vs digital-only pin split, touch-channel (GPIO1–14) mapping, and the GPIO19/20 native-USB-D-/D+ association are standard ESP32-S3 datasheet facts, not sourced from a single URL — used on the GPIO header page (`printHeaderPins()`).
