/**
 * Wireless Info Display - ESP32 E-Paper HMI Display Controller (Standalone Version)
 * This code manages a 4.2" E-paper display. 
 * It defines a system to render up to 8 rows of text with adjustable fonts, borders, and inverted colors
 * It handles the full power cycle: initializing, drawing text, updating, and hibernating.
 * 
 * Based on: 
 * https://github.com/papercodeIN/Elecrow/tree/main/CrowPanel%20-%20ESP32%20E-Paper%20HMI%20Display%20-%204.2%20Inch/Project/Wireless_Text_Display%20-%2023-SEP-2025
 * 
 */

#include "GxEPD2_BW.h"
#include "config.h"

// Pin definitions
#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45

// E-paper display initialization
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

// Display settings structure
struct Row {
  String text;
  int fontSize;
  Row() : text(""), fontSize(18) {}
};

struct DisplaySettings {
  static const int MAX_ROWS = 8;
  Row rows[MAX_ROWS];
  bool border;
  bool invertColors;
  DisplaySettings() : border(true), invertColors(false) {}
};

DisplaySettings currentSettings;

/**
 * Control power to the e-paper display
 */
void epdPower(int state) {
  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, state);
}

/**
 * Initialize the e-paper display with current settings
 */
void epdInit() {
  epd.init(EPD_INIT_BAUD_RATE, EPD_INIT_RESET, EPD_INIT_DELAY, EPD_INIT_PARTIAL);
  epd.setRotation(0);
  epd.setTextColor(currentSettings.invertColors ? GxEPD_WHITE : GxEPD_BLACK);
  epd.setFullWindow();
}

/**
 * Set the font size for e-paper display
 */
void setFontSize(int size) {
  switch (size) {
    case FONT_SIZE_SMALL:
      epd.setFont(&FreeMonoBold12pt7b);
      break;
    case FONT_SIZE_MEDIUM:
      epd.setFont(&FreeMonoBold18pt7b);
      break;
    case FONT_SIZE_LARGE:
      epd.setFont(&FreeMonoBold24pt7b);
      break;
  }
}

/**
 * Get the row height in pixels for a given font size
 */
int getRowHeight(int fontSize) {
  switch (fontSize) {
    case FONT_SIZE_SMALL: return ROW_HEIGHT_SMALL;
    case FONT_SIZE_MEDIUM: return ROW_HEIGHT_MEDIUM;
    case FONT_SIZE_LARGE: return ROW_HEIGHT_LARGE;
    default: return ROW_HEIGHT_MEDIUM;
  }
}

/**
 * Update the e-paper display with current settings
 */
void updateDisplay() {
  epdPower(HIGH);
  epdInit();
  
  // Set background color based on inversion setting
  epd.fillScreen(currentSettings.invertColors ? GxEPD_BLACK : GxEPD_WHITE);
  
  if (currentSettings.border) {
    epd.drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, currentSettings.invertColors ? GxEPD_WHITE : GxEPD_BLACK);
  }
  
  int yPos = getRowHeight(FONT_SIZE_LARGE);
  int margin = DISPLAY_MARGIN;
  
  for (int i = 0; i < currentSettings.MAX_ROWS; i++) {
    if (currentSettings.rows[i].text.length() > 0) {
      setFontSize(currentSettings.rows[i].fontSize);
      epd.setCursor(margin, yPos);
      epd.print(currentSettings.rows[i].text);
      yPos += getRowHeight(currentSettings.rows[i].fontSize);
    }
  }
  
  epd.display();
  epd.hibernate();
  epdPower(LOW);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Display Controller Starting...");

  currentSettings.invertColors = false;
  currentSettings.border = true;
  
  currentSettings.rows[0].text = "Hello World";
  currentSettings.rows[0].fontSize = FONT_SIZE_LARGE;
  
  currentSettings.rows[1].text = "Second line";
  currentSettings.rows[1].fontSize = FONT_SIZE_MEDIUM;


  currentSettings.rows[2].text = "Third line";
  currentSettings.rows[2].fontSize = FONT_SIZE_SMALL;

  currentSettings.rows[3].text = "Line 4";
  currentSettings.rows[3].fontSize = FONT_SIZE_MEDIUM;

  
  currentSettings.rows[4].text = "Line 5";
  currentSettings.rows[4].fontSize = FONT_SIZE_SMALL;

  updateDisplay();
  
  Serial.println("Setup completed successfully");
}

void loop() {
  // No web server to handle
  delay(1000); 
}
