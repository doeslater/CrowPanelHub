#include "GxEPD2_BW.h"

// Pin definitions
#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45
#define MENU 2

#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

void setup() {
  Serial.begin(115200);
  Serial.println("info_screen starting...");

  // TODO: configure MENU as an input (see task.md)
}

void loop() {
  // TODO: detect a MENU press, then draw sketch name / build time /
  // free heap using the same power lifecycle as desk_badge.ino
}
