
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
  epd.setCursor(100, 60);
  epd.print("David Later");

  // Draw the Subtitle (Centered manually)
  epd.setFont(&FreeSansBold12pt7b);
  epd.setCursor(100, 110);
  epd.print("Project Manager");

  // Draw a simple line in the middle
  epd.drawFastHLine(40, 150, 320, GxEPD_BLACK);  // X=40, Y=150, Width=320

  // Draw the Room Number (Pushed to bottom right manually)
  epd.setFont(&FreeSans12pt7b);
  epd.setCursor(260, 260);  // X=290 (far right), Y=260 (near bottom)
  epd.print("Room 404");

  epd.display();
  epd.hibernate();

  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, LOW);
  Serial.println("Display Controller End...");
}


void loop() {
}
