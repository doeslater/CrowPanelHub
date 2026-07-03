#ifndef CONFIG_H
#define CONFIG_H

// Pin definitions (matches display_text/ wiring for this board)
#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45

// Display Configuration
const int DISPLAY_WIDTH = 400;
const int DISPLAY_HEIGHT = 300;

// Wire protocol (phone -> ESP32, one-directional; see CLAUDE.md "Wire protocol").
// Frame layout: [1-byte magic][4-byte LE length][4-byte LE unix epoch seconds]
//               [payload][1-byte checksum]
const uint8_t FRAME_MAGIC = 0xA5;
const size_t PAYLOAD_SIZE = (DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8;  // 15000 bytes, packed 1bpp, MSB-first per row
const unsigned long SERIAL_BAUD_RATE = 115200;
const unsigned long SERIAL_TIMEOUT_MS = 5000;  // generous vs. ~1.3s to transfer 15KB at 115200 baud

// Timestamp field sits between the length and the payload -- the ESP32 has no
// RTC/NTP of its own, so the sender must supply the current time. The Android
// app pre-applies its own local UTC offset before sending, so this value is
// formatted as-is with no timezone math on this side (see WireFrame.kt).
const size_t TIMESTAMP_SIZE = 4;   // uint32_t, unix epoch seconds

#endif  // CONFIG_H
