// Pin definitions
#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45
#define BTN_MENU 2

#include "GxEPD2_BW.h"

// Fonts
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
// #include <Fonts/FreeSans18pt7b.h>
// #include <Fonts/FreeSansBold18pt7b.h>
// #include <Fonts/FreeSans24pt7b.h>
// #include <Fonts/FreeSansBold24pt7b.h>

// Display Initialization
GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

// Button debouncing
const unsigned long COOLDOWN_MS = 300;

// Layout
const int LEFT_MARGIN = 10;

struct Button {
  int pin;
  bool wasPressed = false;
  unsigned long lastActionMs = 0;
};

Button btnMenu = {BTN_MENU};

void setup() {
  Serial.begin(115200);
  Serial.println("info_screen starting...");
  
  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, HIGH);
  
  pinMode(BTN_MENU, INPUT_PULLUP);
  epd.init(115200, true, 50, false);
}

void loop() {
  handleMenuPress();
}

bool pressedAndReady(Button &button, unsigned long now) {
  bool pressed = digitalRead(button.pin) == LOW;     // active-low: LOW means pressed
  bool justPressed = pressed && !button.wasPressed;  // rising edge, not held
  button.wasPressed = pressed;

  bool debounced = (now - button.lastActionMs) > COOLDOWN_MS;
  bool ready = justPressed && debounced;
  if (ready) {
    button.lastActionMs = now;
  }
  return ready;
}

void handleMenuPress() {
  if (!pressedAndReady(btnMenu, millis())) return;
  Serial.println("MENU Button Pressed");

  // 1. Power on
  digitalWrite(PWR, HIGH);
  delay(10);  // let voltage settle before talking to the panel

  // 2. Draw
  drawPage();
  epd.display();
  epd.hibernate();

  // 3. Power off, until the next press
  digitalWrite(PWR, LOW);
}

void drawPage() {
  epd.fillScreen(GxEPD_WHITE);
  epd.setTextColor(GxEPD_BLACK);
  epd.setFont(&FreeSans12pt7b);
  epd.setCursor(LEFT_MARGIN, 32);
  
  printTitle("info_screen");
  
  String buildTimestamp = String(__DATE__) + " " + String(__TIME__);
  printLine(buildTimestamp.c_str());
  
  String getFreeHeap = "Free Heap: " + String(ESP.getFreeHeap()) + " B";
  printLine(getFreeHeap.c_str());
}

void printTitle(const char *title) {
  resetLeftMargin();
  epd.setFont(&FreeSansBold12pt7b);
  epd.println(title);
}

void printLine(const char *lineText) {
  resetLeftMargin();
  epd.setFont(&FreeSans12pt7b);
  epd.println(lineText);
}

void resetLeftMargin() {
  epd.setCursor(LEFT_MARGIN, epd.getCursorY());
}
