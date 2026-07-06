# Reference links

External resources for this project, kept in one place so they don't get duplicated/drift across `README.md`, `CLAUDE.md`, and `MEMORY.md`. Add to this file first; other docs should link here rather than repeating URLs.

## Hardware / product

CrowPanel ESP32 4.2" E-Paper HMI Display (Elecrow, SKU DIE07300S)
[https://www.elecrow.com/crowpanel-esp32-4-2-e-paper-hmi-display-with-400-300-resolution-black-white-color-driven-by-spi-interface.html](https://www.elecrow.com/crowpanel-esp32-4-2-e-paper-hmi-display-with-400-300-resolution-black-white-color-driven-by-spi-interface.html)

CrowPanel ESP32 E-Paper HMI 4.2" Display — Wiki
[https://www.elecrow.com/wiki/CrowPanel_ESP32_E-paper_4.2-inch_HMI_Display.html](https://www.elecrow.com/wiki/CrowPanel_ESP32_E-paper_4.2-inch_HMI_Display.html)

CrowPanel ESP32 E-Paper 4.2" Arduino Tutorial
[https://www.elecrow.com/wiki/CrowPanel_ESP32_E-Paper_4.2-inch_Arduino_Tutorial.html](https://www.elecrow.com/wiki/CrowPanel_ESP32_E-Paper_4.2-inch_Arduino_Tutorial.html)

Elecrow demo code (Arduino IDE)
- Demos: [https://www.elecrow.com/download/product/CrowPanel/E-paper/4.2-DIE07300S/Arduino/Demos.zip](https://www.elecrow.com/download/product/CrowPanel/E-paper/4.2-DIE07300S/Arduino/Demos.zip)
- Examples: [https://www.elecrow.com/download/product/CrowPanel/E-paper/4.2-DIE07300S/Arduino/Examples.zip](https://www.elecrow.com/download/product/CrowPanel/E-paper/4.2-DIE07300S/Arduino/Examples.zip)

Elecrow GitHub (official)
[https://github.com/Elecrow-RD/CrowPanel-ESP32-4.2-E-paper-HMI-Display-with-400-300](https://github.com/Elecrow-RD/CrowPanel-ESP32-4.2-E-paper-HMI-Display-with-400-300)

papercodeIN/Elecrow — source of the `workspace/sketches/display_text/` reference sketch (`Wireless_Text_Display` project)
[https://github.com/papercodeIN/Elecrow](https://github.com/papercodeIN/Elecrow)

## E-paper driver (GxEPD2 / SSD1683)

GxEPD2 — Arduino display library for SPI e-paper displays (used by `workspace/sketches/display_text/display_text.ino`)
[https://github.com/ZinggJM/GxEPD2](https://github.com/ZinggJM/GxEPD2)

CrowPanel 4.2" E-Paper with GxEPD2 (Makerguides tutorial)
[https://www.makerguides.com/crowpanel-4-2-inch-e-paper-with-gxepd2/](https://www.makerguides.com/crowpanel-4-2-inch-e-paper-with-gxepd2/)

SSD1683 eInk display with GxEPD and ESP32 (basics and configuration)
[https://mischianti.org/ssd1683-eink-display-with-gxepd-and-esp32-and-crowpanel-4-2-hmi-basics-and-configuration/](https://mischianti.org/ssd1683-eink-display-with-gxepd-and-esp32-and-crowpanel-4-2-hmi-basics-and-configuration/)

## Firmware toolchain

Arduino core for ESP32 (Espressif) — board package providing `ESP32S3 Dev Module`, used by the current Arduino IDE setup (see `arduino-ide-setup.md`)
[https://github.com/espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)

ESP32 board manager index (Espressif) — the `arduino-cli config add board_manager.additional_urls ...` URL that makes `esp32:esp32:*` boards installable via `arduino-cli core install esp32:esp32` (see `docs/dev-tools.md`)
[https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json](https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json)

PlatformIO — planned firmware toolchain (not yet installed; see `CLAUDE.md`)
[https://platformio.org/](https://platformio.org/) · docs: [https://docs.platformio.org/](https://docs.platformio.org/)

## Python libraries

Pillow (PIL) — used by `workspace/sketches/test_card/generate_test_pattern.py`/`render_preview.py` to build the 400×300 test-card bitmaps sent to (or previewed for) the panel (see `docs/dev-tools.md`)
[https://pillow.readthedocs.io/](https://pillow.readthedocs.io/)

## Android libraries (in use)

usb-serial-for-android (mik3y) — USB serial transport library used by `UsbSerialTransport.kt`; fetched from JitPack (see `docs/dev-tools.md` for the repository/dependency setup)
[https://github.com/mik3y/usb-serial-for-android](https://github.com/mik3y/usb-serial-for-android)

Hilt — dependency injection, used for `CrowPanelHubApp`/`MainViewModel`/`DiagnosticsViewModel`/`UsbSerialTransport` (KSP-based annotation processing, not kapt)
[https://dagger.dev/hilt/](https://dagger.dev/hilt/)

Navigation Compose — provides the two-route `NavHost` (`main`/`diagnostics`) in `MainActivity.kt`
[https://developer.android.com/develop/ui/compose/navigation](https://developer.android.com/develop/ui/compose/navigation)

## Community projects (prior art)

What other developers have built with ESP32 + e-ink — collected while researching `ideas.md`'s idea pool.

lmarzen/esp32-weather-epd — the flagship low-power e-paper weather display (OpenWeatherMap; ~14µA deep sleep, 6–12 months per battery charge)
[https://github.com/lmarzen/esp32-weather-epd](https://github.com/lmarzen/esp32-weather-epd)

G6EJD/ESP32-e-Paper-Weather-Display — weather display supporting 2.9"/4.2"/7.5" panels (including 4.2" like this project's)
[https://github.com/G6EJD/ESP32-e-Paper-Weather-Display](https://github.com/G6EJD/ESP32-e-Paper-Weather-Display)

wuspy/portal_calendar — Portal-themed e-ink calendar, runs for years on AAA batteries
[https://github.com/wuspy/portal_calendar](https://github.com/wuspy/portal_calendar)

kyleturman/home-dashboard — the "server renders the image, board just fetches and displays" architecture (see `ideas.md` idea 18)
[https://github.com/kyleturman/home-dashboard](https://github.com/kyleturman/home-dashboard)

SeBassTian23/ESP32-CalendarDisplay — same server-side-rendering split, calendar-focused
[https://github.com/SeBassTian23/ESP32-CalendarDisplay](https://github.com/SeBassTian23/ESP32-CalendarDisplay)

Hackaday's e-ink tag — the creative fringe, continuously updated (F1 race tracker, a ~60Hz-partial-refresh e-ink Game Boy, the LightInk solar watch that ran 9 months, an ESP32 PDA, Wi-Fi photo frames)
[https://hackaday.com/tag/e-ink/](https://hackaday.com/tag/e-ink/)

TRMNL — commercial-but-open e-ink dashboard (ESP32 + 7.5" panel): server composes the image, board fetches and sleeps for months; 375+ plugins, open firmware, self-hostable server (see `ideas.md` idea 9)
[https://trmnl.com/](https://trmnl.com/) · firmware/server: [https://github.com/usetrmnl](https://github.com/usetrmnl)

OpenEPaperLink — open firmware/protocol for electronic shelf labels: an ESP32 access point pushes images/weather/RSS/calendar to fleets of ~9µA battery e-ink price tags (see `ideas.md` idea 30)
[https://github.com/OpenEPaperLink/OpenEPaperLink](https://github.com/OpenEPaperLink/OpenEPaperLink)

Meshtastic supported devices — the off-grid LoRa mesh world where e-ink is the default display for battery reasons (Heltec Wireless Paper, Vision Master, etc. — see `ideas.md` idea 29)
[https://meshtastic.org/docs/hardware/devices/](https://meshtastic.org/docs/hardware/devices/)
