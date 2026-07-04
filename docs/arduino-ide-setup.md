# Arduino IDE setup for the CrowPanel ESP32-S3 board

Transcribed from `screenshot_2026-01-03_20-34-42.png` (Arduino IDE `Tools` menu), captured during a successful upload of `firmwares/display_text/display_text.ino` (console showed `Done uploading.` / `Hard resetting via RTS pin...`). This is a known-working configuration for this board, not the current machine's installed toolchain (Arduino IDE isn't installed here — see `CLAUDE.md`).

- **Arduino IDE version**: 1.8.19

## Tools menu settings

| Setting | Value |
|---|---|
| Board | `ESP32S3 Dev Module` |
| Upload Speed | `115200` |
| USB Mode | `Hardware CDC and JTAG` |
| USB CDC On Boot | `Disabled` |
| USB Firmware MSC On Boot | `Disabled` |
| USB DFU On Boot | `Disabled` |
| Upload Mode | `UART0 / Hardware CDC` |
| CPU Frequency | `240MHz (WiFi)` |
| Flash Mode | `QIO 80MHz` |
| Flash Size | `4MB (32Mb)` |
| Partition Scheme | `Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)` |
| Core Debug Level | `None` |
| PSRAM | `OPI PSRAM` |
| Arduino Runs On | `Core 1` |
| Events Run On | `Core 1` |
| Erase All Flash Before Sketch Upload | `Disabled` |
| JTAG Adapter | `Disabled` |
| Port | `/dev/ttyUSB0` (Linux; varies by machine/OS) |

Requires the `esp32` board package (for `ESP32S3 Dev Module`) and the GxEPD2 library (plus its font headers) installed via Library Manager — see `CLAUDE.md` Sources for the GxEPD2/SSD1683 reference links.
