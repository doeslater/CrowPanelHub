/**
 * test_card - Self-drawn Philips PM5544-style broadcast test card
 *
 * Draws a test card on boot, no phone or PC needed, showing when this
 * firmware was built (__DATE__/__TIME__, filled in by the compiler at
 * compile time) in the reserved strip -- not the current time, since the
 * ESP32 has no clock of its own, but a fixed value baked into the binary
 * once, at compile time.
 *
 * Also listens for wire-protocol frames over Serial afterwards, same as
 * receive_image.ino, showing a real "Last updated" label once one
 * arrives.
 */

#include <string.h>
#include <time.h>

#include "GxEPD2_BW.h"
#include "config.h"

// __DATE__/__TIME__ are standard C/C++ macros -- the compiler replaces
// them with the date/time the file was compiled, as string literals.
// Concatenating two adjacent string literals like this is plain C/C++,
// not an Arduino-specific trick.
const char *BUILD_TIMESTAMP = "[" __DATE__ " | " __TIME__ "]";

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> display(
    GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

uint8_t payload_buffer[PAYLOAD_SIZE];

// Test card layout. Kept here, not in config.h, since these only matter
// for this one pattern.
const int BORDER_CELL = 20;
const int GRID_CELL = 20;
const int CIRCLE_RADIUS = 105;
const uint8_t BACKGROUND_GRAY = 210;

// Bottom strip reserved for the "last updated" label. The card is drawn to
// fit inside CONTENT_HEIGHT, not the full DISPLAY_HEIGHT, so this strip
// stays blank.
const int LABEL_HEIGHT = 20;
const int CONTENT_HEIGHT = DISPLAY_HEIGHT - LABEL_HEIGHT;

// Circle band heights, as fractions of the diameter. Must add up to 1.0.
const float BAND_FRACTIONS[5] = {0.17f, 0.20f, 0.25f, 0.225f, 0.155f};
const float TOP_SPLIT_WHITE_FRACTION = 0.63f;
const float BOTTOM_SPLIT_BLACK_FRACTION = 0.565f;
const int CHECKERBOARD_BLOCK = 26;

// y_bounds[i] to y_bounds[i+1] is one band's y-range: 0=top split,
// 1=grating, 2=checkerboard, 3=gradient, 4=bottom split.
int y_bounds[6];

void computeBandBounds() {
  int cy = CONTENT_HEIGHT / 2;
  int heights[5];
  int sum = 0;
  for (int i = 0; i < 5; i++) {
    heights[i] = (int)(BAND_FRACTIONS[i] * 2 * CIRCLE_RADIUS + 0.5f);
    sum += heights[i];
  }
  heights[4] += 2 * CIRCLE_RADIUS - sum;  // put any rounding leftover in the last band

  y_bounds[0] = cy - CIRCLE_RADIUS;
  for (int i = 0; i < 5; i++) {
    y_bounds[i + 1] = y_bounds[i] + heights[i];
  }
}

bool insideCircle(int x, int y) {
  int cx = DISPLAY_WIDTH / 2;
  int cy = CONTENT_HEIGHT / 2;
  long dx = x - cx;
  long dy = y - cy;
  return dx * dx + dy * dy <= (long)CIRCLE_RADIUS * CIRCLE_RADIUS;
}

// Left half of the band is black. Right half is white, then black.
uint8_t bandTopSplit(int x, int y, int y0, int y1) {
  int cx = DISPLAY_WIDTH / 2;
  if (x < cx) return 0;
  int rightWhiteBottom = y0 + (int)((y1 - y0) * TOP_SPLIT_WHITE_FRACTION + 0.5f);
  return (y < rightWhiteBottom) ? 255 : 0;
}

// Vertical stripes, wide on the left, narrow on the right.
uint8_t bandGrating(int queryX, int x0) {
  static const int widths[] = {8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 2, 2, 2, 2};
  const int numWidths = sizeof(widths) / sizeof(widths[0]);
  int cur = x0;
  bool black = true;
  for (int i = 0; i < 200; i++) {  // safety limit, band is only ~210px wide
    int w = widths[i < numWidths ? i : numWidths - 1];
    if (queryX < cur + w) return black ? 0 : 255;
    cur += w;
    black = !black;
  }
  return 255;  // never reached
}

uint8_t bandCheckerboard(int x, int y, int x0, int x1, int y0, int y1) {
  int cols = max(1, (int)((x1 - x0) / (float)CHECKERBOARD_BLOCK + 0.5f));
  int rows = max(1, (int)((y1 - y0) / (float)CHECKERBOARD_BLOCK + 0.5f));
  float colW = (x1 - x0) / (float)cols;
  float rowH = (y1 - y0) / (float)rows;
  int col = (int)((x - x0) / colW);
  int row = (int)((y - y0) / rowH);
  return ((row + col) % 2 == 0) ? 0 : 255;
}

// The only band with real gray values. renderPM5544Card()'s dithering
// turns this into a black/white halftone.
uint8_t bandGradient(int x, int x0, int x1) {
  static const uint8_t steps[] = {30, 90, 150, 210, 255};
  const int n = 5;
  float stepW = (x1 - x0) / (float)n;
  int idx = (int)((x - x0) / stepW);
  if (idx >= n) idx = n - 1;
  if (idx < 0) idx = 0;
  return steps[idx];
}

// Same idea as bandTopSplit, flipped (the card is symmetric if you rotate
// it 180 degrees).
uint8_t bandBottomSplit(int x, int y, int y0, int y1) {
  int cx = DISPLAY_WIDTH / 2;
  int blackBottom = y0 + (int)((y1 - y0) * BOTTOM_SPLIT_BLACK_FRACTION + 0.5f);
  if (y < blackBottom) return 0;
  return (x < cx) ? 255 : 0;
}

// Picks which band owns row y, for a pixel already known to be inside the
// circle.
uint8_t circleContentAt(int x, int y) {
  int cx = DISPLAY_WIDTH / 2;
  int x0 = cx - CIRCLE_RADIUS;
  int x1 = cx + CIRCLE_RADIUS;
  for (int i = 0; i < 5; i++) {
    if (y >= y_bounds[i] && y < y_bounds[i + 1]) {
      switch (i) {
        case 0: return bandTopSplit(x, y, y_bounds[0], y_bounds[1]);
        case 1: return bandGrating(x, x0);
        case 2: return bandCheckerboard(x, y, x0, x1, y_bounds[2], y_bounds[3]);
        case 3: return bandGradient(x, x0, x1);
        default: return bandBottomSplit(x, y, y_bounds[4], y_bounds[5]);
      }
    }
  }
  return 255;  // never reached, y_bounds always covers the full circle
}

// Only the outer ring of border cells is ever drawn -- inner cells would
// just get covered by the plain gray background anyway.
bool borderCellColor(int x, int y, uint8_t *outColor) {
  int col = x / BORDER_CELL;
  int row = y / BORDER_CELL;
  int cols = DISPLAY_WIDTH / BORDER_CELL;
  int rows = CONTENT_HEIGHT / BORDER_CELL;
  if (col > 0 && col < cols - 1 && row > 0 && row < rows - 1) return false;
  *outColor = ((col + row) % 2 == 0) ? 0 : 255;
  return true;
}

void setBlackPixel(uint8_t *payload, int x, int y) {
  size_t rowStart = y * (DISPLAY_WIDTH / 8);
  size_t byteIndex = rowStart + x / 8;
  uint8_t bit = 7 - (x % 8);
  payload[byteIndex] |= (1 << bit);
}

void clearWhitePixel(uint8_t *payload, int x, int y) {
  size_t rowStart = y * (DISPLAY_WIDTH / 8);
  size_t byteIndex = rowStart + x / 8;
  uint8_t bit = 7 - (x % 8);
  payload[byteIndex] &= ~(1 << bit);
}

/**
 * Builds the card and dithers it straight into `payload`, one row at a
 * time. No separate gray-scale image is ever built -- each pixel's color
 * is worked out and dithered right when it's needed. Uses the standard
 * Floyd-Steinberg dithering weights, carrying leftover error forward in
 * two small row buffers (errCurr, errNext) instead of a full 2D buffer.
 */
void renderPM5544Card(uint8_t *payload) {
  computeBandBounds();
  memset(payload, 0x00, PAYLOAD_SIZE);  // start all white

  int16_t errCurr[DISPLAY_WIDTH];
  int16_t errNext[DISPLAY_WIDTH];
  memset(errCurr, 0, sizeof(errCurr));

  for (int y = 0; y < CONTENT_HEIGHT; y++) {  // rows below this stay white -- the label strip
    memset(errNext, 0, sizeof(errNext));
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
      uint8_t idealGray;
      uint8_t borderColor;
      if (insideCircle(x, y)) {
        idealGray = circleContentAt(x, y);
      } else if (borderCellColor(x, y, &borderColor)) {
        idealGray = borderColor;
      } else {
        idealGray = BACKGROUND_GRAY;
      }

      int16_t value = (int16_t)idealGray + errCurr[x];
      value = constrain(value, 0, 255);
      uint8_t chosen = (value < 128) ? 0 : 255;  // 0 = black, 255 = white
      int16_t err = value - chosen;

      if (x + 1 < DISPLAY_WIDTH) errCurr[x + 1] += (err * 7) / 16;
      if (x > 0) errNext[x - 1] += (err * 3) / 16;
      errNext[x] += (err * 5) / 16;
      if (x + 1 < DISPLAY_WIDTH) errNext[x + 1] += (err * 1) / 16;

      if (chosen == 0) setBlackPixel(payload, x, y);
    }
    memcpy(errCurr, errNext, sizeof(errCurr));
  }

  drawGridLines(payload);
}

// Draws white grid lines over the dithered background, skipping the
// circle. Done after dithering so the lines stay sharp.
void drawGridLines(uint8_t *payload) {
  int left = BORDER_CELL, top = BORDER_CELL;
  int right = DISPLAY_WIDTH - BORDER_CELL, bottom = CONTENT_HEIGHT - BORDER_CELL;

  for (int x = left; x <= right; x += GRID_CELL) {
    for (int y = top; y <= bottom; y++) {
      for (int dx = 0; dx < 2; dx++) {  // 2px wide, so it's still visible
        int px = x + dx;
        if (px > right) continue;
        if (!insideCircle(px, y)) clearWhitePixel(payload, px, y);
      }
    }
  }
  for (int y = top; y <= bottom; y += GRID_CELL) {
    for (int x = left; x <= right; x++) {
      for (int dy = 0; dy < 2; dy++) {
        int py = y + dy;
        if (py > bottom) continue;
        if (!insideCircle(x, py)) clearWhitePixel(payload, x, py);
      }
    }
  }
}

// ============================================================================
// Display + serial receive loop
// ============================================================================

void renderPayload(const uint8_t *payload, const char *label) {
  display.fillScreen(GxEPD_WHITE);
  display.drawBitmap(0, 0, payload, DISPLAY_WIDTH, DISPLAY_HEIGHT, GxEPD_BLACK);
  // Always clear the label strip, even with no label yet. A received
  // frame's payload fills the whole screen and isn't blank there.
  display.fillRect(0, CONTENT_HEIGHT, DISPLAY_WIDTH, LABEL_HEIGHT, GxEPD_WHITE);
  if (label != NULL) {
    display.setTextSize(2);  // default size 1 is small and hard to read

    // getTextBounds() measures how wide `label` will actually be drawn
    // (depends on its length and the current text size), so the cursor
    // can be centered instead of always starting at a fixed x. x1 is
    // subtracted because it's the glyph's own left-side offset from the
    // cursor -- without it, centering would be off by that amount.
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    display.getTextBounds(label, 0, 0, &x1, &y1, &textWidth, &textHeight);
    int16_t centeredX = (DISPLAY_WIDTH - textWidth) / 2 - x1;

    display.setCursor(centeredX, CONTENT_HEIGHT + 4);
    display.print(label);
  }
  display.display(false);
}

bool receiveFrame(uint32_t *out_timestamp) {
  if (Serial.available() <= 0) return false;

  bool synced = false;
  while (Serial.available() > 0) {
    if ((uint8_t)Serial.read() == FRAME_MAGIC) {
      synced = true;
      break;
    }
  }
  if (!synced) return false;

  uint8_t header[4 + TIMESTAMP_SIZE];
  if (Serial.readBytes(header, sizeof(header)) != sizeof(header)) {
    Serial.println("frame dropped: header read timed out");
    return false;
  }

  uint32_t length;
  memcpy(&length, header, 4);
  if (length != PAYLOAD_SIZE) {
    Serial.println("frame dropped: bad length");
    return false;
  }
  memcpy(out_timestamp, header + 4, TIMESTAMP_SIZE);

  if (Serial.readBytes(payload_buffer, PAYLOAD_SIZE) != PAYLOAD_SIZE) {
    Serial.println("frame dropped: payload read timed out");
    return false;
  }

  uint8_t checksum_byte;
  if (Serial.readBytes(&checksum_byte, 1) != 1) {
    Serial.println("frame dropped: checksum read timed out");
    return false;
  }

  uint8_t checksum = 0;
  for (size_t i = 0; i < PAYLOAD_SIZE; i++) {
    checksum += payload_buffer[i];
  }
  if (checksum != checksum_byte) {
    Serial.println("frame dropped: checksum mismatch");
    return false;
  }

  return true;
}

void setup() {
  // Must run before Serial.begin(). Drawing the boot card takes a while,
  // and loop() can't read Serial until setup() finishes -- so the buffer
  // needs to be big enough to hold a whole frame while it waits.
  Serial.setRxBufferSize(PAYLOAD_SIZE + 64);
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.setTimeout(SERIAL_TIMEOUT_MS);

  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, HIGH);

  display.init(SERIAL_BAUD_RATE, true, 50, false);
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);
  display.setFullWindow();

  renderPM5544Card(payload_buffer);
  renderPayload(payload_buffer, BUILD_TIMESTAMP);

  Serial.println("test_card ready, waiting for frames");
}

void loop() {
  uint32_t timestamp;
  if (!receiveFrame(&timestamp)) return;

  time_t ts = (time_t)timestamp;
  struct tm tm_info;
  localtime_r(&ts, &tm_info);
  char label[40];  // needs 34 bytes; a smaller buffer would silently cut the text short
  strftime(label, sizeof(label), "Last updated: %Y-%m-%d %H:%M:%S", &tm_info);

  Serial.print("frame ok, ");
  Serial.println(label);
  renderPayload(payload_buffer, label);
}
