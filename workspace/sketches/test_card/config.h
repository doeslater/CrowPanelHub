#ifndef CONFIG_H
#define CONFIG_H

// config_h.py (this folder) parses this file's `const <type> NAME = <expr>;`
// lines directly, so this repo's Python scripts never hardcode a value
// that's already defined here. Keep new constants in that same form (not
// #define) if they need to be visible to Python too.

// Pins wired to the e-paper panel:
//   PWR   turns panel power on/off
//   BUSY  panel pulls this HIGH while it's redrawing
//   RES   hardware reset
//   DC    tells the panel if this byte is a command or pixel data
//   CS    SPI chip-select
#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45

// Display Configuration
const int DISPLAY_WIDTH = 400;
const int DISPLAY_HEIGHT = 300;

// Wire protocol (PC/phone -> ESP32, one-directional). Same protocol as
// receive_image/config.h -- see CLAUDE.md's "Wire protocol" section.
// Frame layout: [1-byte magic][4-byte LE length][4-byte LE unix epoch seconds]
//               [payload][1-byte checksum]
const uint8_t FRAME_MAGIC = 0xA5;
const size_t PAYLOAD_SIZE = (DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8;  // 15000 bytes, 1 bit per pixel
const unsigned long SERIAL_BAUD_RATE = 115200;
const unsigned long SERIAL_TIMEOUT_MS = 5000;

// The ESP32 has no clock of its own, so the sender includes the current
// time here. It's used as-is, no timezone conversion.
const size_t TIMESTAMP_SIZE = 4;   // uint32_t, unix epoch seconds

#endif  // CONFIG_H
