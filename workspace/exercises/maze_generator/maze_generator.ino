#include <Arduino.h>
#include "GxEPD2_BW.h"

// CrowPanel ESP32-S3 Hardware Pin Definitions
#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45

#include <Fonts/FreeSansBold12pt7b.h>

// 4.2" CrowPanel display initialization
GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

const int COLS = 16;
const int ROWS = 10;
const int CELL = 20;
const int ORIGIN_X = 40;
const int ORIGIN_Y = 70;

// Binary tree maze algorithm: for each cell, carve into whichever of its
// North/West neighbors is available (both already exist in scan order, so
// no visited-tracking or backtracking is needed at all). Guarantees a
// fully-connected, loop-free maze, at the cost of long straight corridors
// along the top row and left column.
void drawMaze() {
  epd.fillRect(ORIGIN_X, ORIGIN_Y, COLS * CELL, ROWS * CELL, GxEPD_BLACK);

  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int px = ORIGIN_X + x * CELL;
      int py = ORIGIN_Y + y * CELL;

      epd.fillRect(px + 2, py + 2, CELL - 4, CELL - 4, GxEPD_WHITE);

      bool carveNorth = (y > 0) && (x == 0 || random(0, 2) == 0);

      if (carveNorth) {
        epd.fillRect(px + 2, py - 2, CELL - 4, 4, GxEPD_WHITE);
      } else if (x > 0) {
        epd.fillRect(px - 2, py + 2, 4, CELL - 4, GxEPD_WHITE);
      }
    }
  }
}

void setup() {
  randomSeed(micros());
  Serial.begin(115200);

  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, HIGH);
  delay(50);

  epd.init(115200, true, 50, false);
  epd.fillScreen(GxEPD_WHITE);
  epd.setTextColor(GxEPD_BLACK);

  epd.setFont(&FreeSansBold12pt7b);
  epd.setCursor(50, 40);
  epd.print("Maze Generator");

  drawMaze();

  epd.display();
  epd.hibernate();

  digitalWrite(PWR, LOW);  // Save power
}

void loop() {
  // Static display
}
