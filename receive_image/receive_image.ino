/**
 * Milestone 1 - Receive Image over USB Serial
 *
 * Reads one length-prefixed bitmap frame from Serial (see config.h for the wire
 * protocol), verifies its checksum, and renders it to the e-paper panel with a
 * full refresh. Fire-and-forget: no acknowledgement is ever sent back to the
 * phone. Power-cycle/init pattern borrowed from display_text/display_text.ino.
 */

#include <time.h>

#include "GxEPD2_BW.h"
#include "config.h"

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

uint8_t frameBuffer[PAYLOAD_SIZE];

void epdPower(int state) {
  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, state);
}

// Labels the frame with the wall-clock time/date the sender supplied -- the
// ESP32 has no RTC or Wi-Fi/NTP of its own (milestone 1 is USB-serial only),
// so epochSeconds must come from the frame every time. Formatted as-is with
// no timezone math -- the Android sender pre-applies its own local offset
// before sending (see WireFrame.kt), so this is already local time.
void drawUpdatedLabel(uint32_t epochSeconds) {
  time_t epoch = (time_t)epochSeconds;
  struct tm timeInfo;
  gmtime_r(&epoch, &timeInfo);

  char label[32];
  strftime(label, sizeof(label), "%Y-%m-%d %H:%M:%S", &timeInfo);

  const int labelHeight = 20;
  const int labelY = DISPLAY_HEIGHT - labelHeight;
  epd.fillRect(0, labelY, DISPLAY_WIDTH, labelHeight, GxEPD_WHITE);
  epd.setTextColor(GxEPD_BLACK);
  epd.setTextSize(2);
  epd.setCursor(5, labelY + 4);
  epd.print(label);
}

void renderFrame(uint32_t epochSeconds) {
  epdPower(HIGH);
  epd.init(SERIAL_BAUD_RATE, true, 50, false);
  epd.setRotation(0);
  epd.setFullWindow();
  epd.drawBitmap(0, 0, frameBuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, GxEPD_BLACK, GxEPD_WHITE);
  drawUpdatedLabel(epochSeconds);
  epd.display();
  epd.hibernate();
  epdPower(LOW);
}

uint8_t checksumOf(const uint8_t *data, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return sum;
}

// Blocks until `len` bytes arrive or Serial's configured timeout elapses.
bool readExact(uint8_t *dest, size_t len) {
  return Serial.readBytes(dest, len) == len;
}

void handleFrame() {
  uint8_t lengthBytes[4];
  if (!readExact(lengthBytes, sizeof(lengthBytes))) {
    Serial.println("Timed out reading frame length");
    return;
  }
  uint32_t length = (uint32_t)lengthBytes[0]
                   | ((uint32_t)lengthBytes[1] << 8)
                   | ((uint32_t)lengthBytes[2] << 16)
                   | ((uint32_t)lengthBytes[3] << 24);

  if (length != PAYLOAD_SIZE) {
    Serial.printf("Unexpected payload length %u, expected %u\n", (unsigned)length, (unsigned)PAYLOAD_SIZE);
    return;
  }

  uint8_t timestampBytes[TIMESTAMP_SIZE];
  if (!readExact(timestampBytes, TIMESTAMP_SIZE)) {
    Serial.println("Timed out reading timestamp");
    return;
  }
  uint32_t epochSeconds = (uint32_t)timestampBytes[0]
                         | ((uint32_t)timestampBytes[1] << 8)
                         | ((uint32_t)timestampBytes[2] << 16)
                         | ((uint32_t)timestampBytes[3] << 24);

  if (!readExact(frameBuffer, PAYLOAD_SIZE)) {
    Serial.println("Timed out reading payload");
    return;
  }

  uint8_t checksumByte;
  if (!readExact(&checksumByte, 1)) {
    Serial.println("Timed out reading checksum");
    return;
  }

  uint8_t expected = checksumOf(frameBuffer, PAYLOAD_SIZE);
  if (checksumByte != expected) {
    Serial.printf("Checksum mismatch: got 0x%02X, expected 0x%02X\n", checksumByte, expected);
    return;
  }

  Serial.println("Frame received, rendering...");
  renderFrame(epochSeconds);
  Serial.println("Render complete");
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.setTimeout(SERIAL_TIMEOUT_MS);
  Serial.println("Receive Image Controller Starting...");
}

void loop() {
  if (Serial.available() > 0 && Serial.read() == FRAME_MAGIC) {
    handleFrame();
  }
  // Any byte that isn't the magic is silently discarded, keeping the reader
  // in sync if it starts mid-frame or receives noise.
}
