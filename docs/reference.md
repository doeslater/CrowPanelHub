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

papercodeIN/Elecrow — source of the `firmwares/display_text/` reference sketch (`Wireless_Text_Display` project)
[https://github.com/papercodeIN/Elecrow](https://github.com/papercodeIN/Elecrow)

## E-paper driver (GxEPD2 / SSD1683)

GxEPD2 — Arduino display library for SPI e-paper displays (used by `firmwares/display_text/display_text.ino`)
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

Pillow (PIL) — used by `firmwares/receive_image/send_text.py` to rasterize text into the 400×300 bitmap sent to the panel (see `docs/dev-tools.md`)
[https://pillow.readthedocs.io/](https://pillow.readthedocs.io/)

## Android libraries (in use)

usb-serial-for-android (mik3y) — USB serial transport library used by `UsbSerialTransport.kt`; fetched from JitPack (see `docs/dev-tools.md` for the repository/dependency setup)
[https://github.com/mik3y/usb-serial-for-android](https://github.com/mik3y/usb-serial-for-android)

Hilt — dependency injection, used for `CrowPanelHubApp`/`MainViewModel`/`DiagnosticsViewModel`/`UsbSerialTransport` (KSP-based annotation processing, not kapt)
[https://dagger.dev/hilt/](https://dagger.dev/hilt/)

Navigation Compose — provides the two-route `NavHost` (`main`/`diagnostics`) in `MainActivity.kt`
[https://developer.android.com/develop/ui/compose/navigation](https://developer.android.com/develop/ui/compose/navigation)
