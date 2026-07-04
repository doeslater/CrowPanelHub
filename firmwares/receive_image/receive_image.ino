/**
 * Milestone 1 - Receive Image over USB Serial
 *
 * Reads one length-prefixed bitmap frame from Serial (see config.h for the wire
 * protocol), verifies its checksum, and renders it to the e-paper panel with a
 * full refresh. Fire-and-forget: no acknowledgement is ever sent back to the
 * phone. Power-cycle/init pattern borrowed from firmwares/display_text/display_text.ino.
 */

#include <time.h>

#include "GxEPD2_BW.h"
#include "config.h"

// GxEPD2_BW is a C++ template class (the driver library supports many panel
// models, and templates let it generate code specific to just this one at
// compile time instead of deciding at runtime). GxEPD2_420_GYE042A87 is the
// specific panel/controller variant for this board; HEIGHT tells the driver
// how many rows of RAM to allocate for its internal page buffer. The
// constructor's CS/DC/RES/BUSY arguments wire it to the pins from config.h.
GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

// A plain global array, not malloc'd/heap-allocated -- its size (15,000
// bytes) is known at compile time, and reserving it once up front avoids ever
// fragmenting the ESP32's heap by repeatedly allocating/freeing a
// same-sized buffer on every frame.
uint8_t frameBuffer[PAYLOAD_SIZE];

// The panel's power pin is wired to a MOSFET switch (see config.h), not the
// panel's logic directly -- pulling it LOW cuts power to the whole panel
// between frames so it draws ~0 current while idle, rather than relying on
// the panel's own (less complete) low-power sleep mode alone.
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
  // time_t/struct tm/strftime are standard C time functions (from <time.h>),
  // not anything ESP32- or Arduino-specific. gmtime_r splits the raw seconds
  // count into year/month/day/hour/etc fields (the "_r" means it writes into
  // a struct you provide, safe to call from anywhere, unlike plain gmtime());
  // strftime then formats those fields into the human-readable string in
  // `label` using the same format codes as Python's/Linux's strftime.
  time_t epoch = (time_t)epochSeconds;
  struct tm timeInfo;
  gmtime_r(&epoch, &timeInfo);

  char label[32];
  strftime(label, sizeof(label), "%Y-%m-%d %H:%M:%S", &timeInfo);

  // Everything below draws into the display's in-memory frame buffer, not
  // the physical panel -- nothing is actually visible until display() runs
  // (see renderFrame()). fillRect blanks out just the bottom strip so the
  // label has a clean background instead of drawing text over the bitmap.
  const int labelHeight = 20;
  const int labelY = DISPLAY_HEIGHT - labelHeight;
  epd.fillRect(0, labelY, DISPLAY_WIDTH, labelHeight, GxEPD_WHITE);
  epd.setTextColor(GxEPD_BLACK);
  epd.setTextSize(2);       // scales the built-in font up 2x so it's readable
  epd.setCursor(5, labelY + 4);
  epd.print(label);
}

// The full power-on -> draw -> refresh -> power-off cycle for one frame.
// E-paper only draws pixels when explicitly told to refresh (display()) --
// unlike an LCD, it holds its last image with zero power in between, which
// is exactly why this firmware can safely cut power to the panel entirely
// between frames instead of leaving it running.
void renderFrame(uint32_t epochSeconds) {
  epdPower(HIGH);
  // init()'s arguments: baud rate for the library's own debug logging (not
  // this sketch's Serial port), whether to hardware-reset the panel via RES,
  // a reset pulse width in milliseconds (50ms -- this board's controller
  // needs longer than the library's 10ms default), and whether to use
  // partial-refresh mode (false = full refresh only, see CLAUDE.md for why).
  epd.init(SERIAL_BAUD_RATE, true, 50, false);
  epd.setRotation(0);
  // Tells the library the *entire* panel is about to be redrawn (as opposed
  // to a smaller sub-rectangle via setPartialWindow(), used by displays that
  // refresh only one region at a time) -- required before any draw call.
  epd.setFullWindow();
  // GxEPD_BLACK/GxEPD_WHITE here are the two colors to draw for a 1 bit vs. a
  // 0 bit in frameBuffer, respectively -- see drawBitmap's bit convention in
  // CLAUDE.md's wire protocol notes.
  epd.drawBitmap(0, 0, frameBuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, GxEPD_BLACK, GxEPD_WHITE);
  drawUpdatedLabel(epochSeconds);
  // This is the one call that actually pushes pixels to the physical panel --
  // everything above only modified an in-memory buffer. It's slow (multiple
  // seconds) and blocks the whole sketch, which is fine here since nothing
  // else needs the CPU while a frame is rendering.
  epd.display();
  // Puts the panel controller into its own lowest-power sleep state before
  // epdPower(LOW) cuts its power rail entirely -- skipping this step before
  // a power cut can leave the controller in an undefined state next boot.
  epd.hibernate();
  epdPower(LOW);
}

// A checksum lets the receiver detect a corrupted/incomplete transfer (e.g.
// a dropped byte from a flaky USB cable) without needing anything fancy: the
// sender computes this same sum over the same bytes and includes it in the
// frame, and handleFrame() below rejects the frame if the two don't match.
// `sum` is a uint8_t, so this addition wraps around (mod 256) on overflow --
// deliberate, not a bug, since the sender's checksum math must wrap the same
// way for the two to ever agree.
uint8_t checksumOf(const uint8_t *data, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return sum;
}

// Serial.readBytes() already blocks (pauses this function) until either
// `len` bytes have arrived or SERIAL_TIMEOUT_MS passes with no more data --
// it returns however many bytes it actually got, which is less than `len`
// on a timeout. This wrapper just turns that into a clean true/false so
// callers below don't have to compare against `len` themselves each time.
bool readExact(uint8_t *dest, size_t len) {
  return Serial.readBytes(dest, len) == len;
}

// Runs once loop() has already found the FRAME_MAGIC sync byte -- reads and
// validates the rest of one frame (length, timestamp, payload, checksum), in
// that wire order, bailing out early (with just a log line, no reply sent
// back) the moment anything looks wrong.
void handleFrame() {
  uint8_t lengthBytes[4];
  if (!readExact(lengthBytes, sizeof(lengthBytes))) {
    Serial.println("Timed out reading frame length");
    return;
  }
  // The wire protocol stores multi-byte numbers little-endian (least
  // significant byte first) -- this is the standard way to reassemble one:
  // each byte is shifted left by 8 more bits than the last (byte 0 stays put,
  // byte 1 moves up 8 bits, etc.) and OR'd together into a single uint32_t.
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
  // Same little-endian reassembly as `length` above, this time for the unix
  // epoch seconds value the sender embedded (see config.h's TIMESTAMP_SIZE).
  uint32_t epochSeconds = (uint32_t)timestampBytes[0]
                         | ((uint32_t)timestampBytes[1] << 8)
                         | ((uint32_t)timestampBytes[2] << 16)
                         | ((uint32_t)timestampBytes[3] << 24);

  // Reads straight into the same buffer renderFrame() will draw from later --
  // no separate copy needed since nothing else uses frameBuffer in between.
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

// setup() runs once at boot/reset; loop() then runs over and over forever --
// the standard Arduino sketch shape. Deliberately kept fast here: unlike
// test firmware that might draw something on boot, this setup() does no
// panel work at all, so loop() (and therefore Serial reception) starts
// almost immediately after power-on.
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.setTimeout(SERIAL_TIMEOUT_MS);   // applies to every Serial.readBytes() call below
  Serial.println("Receive Image Controller Starting...");
}

void loop() {
  // Serial.available() checks whether at least one byte has arrived without
  // blocking; Serial.read() then consumes exactly one byte. Only once that
  // byte matches FRAME_MAGIC does this hand off to handleFrame() to read the
  // rest -- loop() itself runs continuously, so this check re-happens on
  // every pass whether or not a byte is waiting.
  if (Serial.available() > 0 && Serial.read() == FRAME_MAGIC) {
    handleFrame();
  }
  // Any byte that isn't the magic is silently discarded, keeping the reader
  // in sync if it starts mid-frame or receives noise.
}
