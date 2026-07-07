
#include "GxEPD2_BW.h"
// #include "config.h"

// Pin definitions
#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45

// E-paper display initialization
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

void setup() {
  // Random
  randomSeed(micros());

  Serial.begin(115200);
  Serial.println("Display Controller Starting...");

  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, HIGH);

  epd.init(115200, true, 50, false);

  // Clear the screen to white
  epd.fillScreen(GxEPD_WHITE);
  epd.setTextColor(GxEPD_BLACK);

  // Draw the Title (Centered manually)
  epd.setFont(&FreeSansBold18pt7b);
  epd.setCursor(100, 40);
  epd.print("Gen Art");

  // Rectangle
  epd.drawRect(40, 30, 320, 240, GxEPD_BLACK);

  // Outer circle
  epd.fillCircle(200, 150, 50, GxEPD_BLACK);
  // Inner circle
  epd.fillCircle(200, 150, 20, GxEPD_WHITE);

  // Rect
  epd.fillRect(199, 118, 3, 13, GxEPD_WHITE);
  epd.fillRect(199, 175, 3, 13, GxEPD_WHITE);
  epd.fillRect(168, 149, 13, 3, GxEPD_WHITE);
  epd.fillRect(225, 149, 13, 3, GxEPD_WHITE);

  // Square top right
  epd.fillRect(300, 40, 40, 40, GxEPD_BLACK);

  // Triangle bottom right
  epd.drawTriangle(320, 240, 290, 200, 350, 200, GxEPD_BLACK);

  // Small circle top left
  epd.drawCircle(40, 70, 10, GxEPD_BLACK);

  // Filled circle bottom left
  epd.fillCircle(40, 120, 10, GxEPD_BLACK);

  epd.drawRect(40, 30, 320, 240, GxEPD_BLACK);
  
  for (int i = 0; i < 30; i++) {
    int x1 = random(40, 361);
    int y1 = random(30, 271);
    int x2 = random(40, 361);
    int y2 = random(30, 271);
    epd.drawLine(x1, y1, x2, y2, GxEPD_BLACK);
  }
  
  epd.display();
  epd.hibernate();

  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, LOW);
  Serial.println("Display Controller End...");
}


void loop() {
}
