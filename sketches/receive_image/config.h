#ifndef CONFIG_H
#define CONFIG_H

// Pin definitions (matches display_text/ wiring for this board). These are
// ESP32-S3 GPIO numbers, each wired to one signal the e-paper panel needs:
//   PWR   a GPIO driving a MOSFET that switches power to the panel on/off --
//         letting firmware fully cut power between refreshes to save energy.
//   BUSY  an input from the panel: it holds this line HIGH while it's
//         physically redrawing, so the GxEPD2 library can poll it and wait.
//   RES   hardware reset for the panel's controller chip.
//   DC    "data/command" -- SPI has no separate signal for "this byte is a
//         command vs. pixel data", so this extra pin tells the panel which
//         one is being sent.
//   CS    SPI chip-select -- pulled low to say "this SPI traffic is for you"
//         (relevant if other SPI devices ever shared the bus).
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
//
// FRAME_MAGIC exists purely so the receiver can find the *start* of a frame:
// without it, if the ESP32's serial reader started listening mid-transmission
// (e.g. it rebooted partway through a send), it would misinterpret whatever
// bytes it starts seeing as a length/timestamp/payload and get hopelessly out
// of sync. Scanning for one known byte value first re-establishes a known
// starting point. It's not a "sender identity" check -- see CLAUDE.md for how
// a matching checksum-mismatch/timeout is handled if things still go wrong.
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
