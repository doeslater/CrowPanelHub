// Hardware inventory sketch for the CrowPanel ESP32-S3 4.2" e-paper HMI board.
// Renders chip/memory/radio info, pin-map references, and live diagnostics
// (uptime, temperature, flash partitions, a WiFi scan) as a set of pages on
// the e-paper panel itself. UP/DOWN page through them, MENU jumps home, EXIT
// forces a full refresh. Rotated 90 degrees to portrait with a smaller font
// so each section stays legible.

// Pin assignments below are hardware-verified for this exact board (confirmed
// against github.com/doeslater/CrowPanelHub's display_text.ino, which drives
// the panel with GxEPD2 on these same pins). SCK/MOSI for the panel's SPI bus
// aren't pinned down in that repo (GxEPD2 there only names CS/DC/RST/BUSY/PWR),
// so they're sourced from Elecrow's published pinout instead and marked below.

#include "GxEPD2_BW.h"
#include <esp_system.h>       // esp_reset_reason()
#include <esp_chip_info.h>    // esp_chip_info()
#include <esp_mac.h>          // esp_read_mac()
#include <esp_partition.h>    // esp_partition_find() / esp_partition_next()
#include <WiFi.h>             // WiFi.scanNetworks() -- only page 12 uses the radio

// E-paper panel pins
#define PWR 7
#define BUSY 48
#define RES 47
#define DC 46
#define CS 45

// Rotary/menu buttons. Assumed active-LOW with internal pull-up (the common
// convention for this class of board) -- not independently confirmed against
// this board's schematic. If a button's effect is reversed from what's
// expected once flashed, swap its role in pollButtons().
#define BTN_UP 6
#define BTN_DOWN 4
#define BTN_MENU 2   // jumps to page 1 (home)
#define BTN_EXIT 1   // forces a full (non-partial) refresh of the current page
#define BTN_OK 5     // reserved, no action yet

#include <Fonts/FreeSans9pt7b.h>             // sans-serif, regular, 9pt -- body text
#include <Fonts/FreeSansBold9pt7b.h>         // sans-serif, bold, 9pt -- printLine()'s labels
#include <Fonts/FreeSans12pt7b.h>            // sans-serif, regular, 12pt -- section titles
#include <Fonts/FreeSerifBoldItalic24pt7b.h>  // serif, bold italic, 24pt -- page header

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> epd(GxEPD2_420_GYE042A87(CS, DC, RES, BUSY));

const int PAGE_COUNT = 12;
const int LEFT_MARGIN = 10;
int currentPage = 0;

const unsigned long PAGE_CHANGE_COOLDOWN_MS = 300;  // basic mechanical-bounce guard

// Groups each button's pin with the state pressedAndReady() needs to track,
// instead of five parallel "lastXPressed" bools and four parallel "xEdgeMs"
// timers.
struct Button {
  int pin;
  bool wasPressed = false;
  unsigned long lastActionMs = 0;
};

Button btnUp = {BTN_UP};
Button btnDown = {BTN_DOWN};
Button btnMenu = {BTN_MENU};
Button btnExit = {BTN_EXIT};
Button btnOk = {BTN_OK};  // reserved, no action yet


void setup() {
  Serial.begin(115200);
  Serial.println("Display Controller Starting...");

  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, HIGH);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_EXIT, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);

  epd.init(115200, true, 50, false);
  epd.setRotation(1);  // 90 degrees clockwise: 400x300 landscape -> 300x400 portrait

  // First draw is a full refresh so later partial updates have a clean
  // baseline to diff against; page changes after this use display(true).
  drawPage(currentPage, false);

  Serial.println("Ready -- rotary UP/DOWN pages through the report.");
}

void loop() {
  pollButtons();
}

// True once per physical press (release->press transition), and only if
// this button's own mechanical-bounce cooldown has elapsed. Combines what
// used to be a separate edge-detect helper plus a repeated cooldown check
// on every branch below into one call per button.
bool pressedAndReady(Button &button, unsigned long now) {
  bool pressed = digitalRead(button.pin) == LOW;
  bool edge = pressed && !button.wasPressed;
  button.wasPressed = pressed;

  bool ready = edge && (now - button.lastActionMs) > PAGE_CHANGE_COOLDOWN_MS;
  if (ready) {
    button.lastActionMs = now;
  }
  return ready;
}

void pollButtons() {
  unsigned long now = millis();

  // Sample every button first, so held/released state stays accurate for
  // all of them each pass, then act on whichever one actually fired.
  bool upReady = pressedAndReady(btnUp, now);
  bool downReady = pressedAndReady(btnDown, now);
  bool menuReady = pressedAndReady(btnMenu, now);
  bool exitReady = pressedAndReady(btnExit, now);
  pressedAndReady(btnOk, now);  // reserved, no action yet

  if (upReady) {
    currentPage = (currentPage - 1 + PAGE_COUNT) % PAGE_COUNT;
    Serial.println("UP -> page " + String(currentPage + 1) + "/" + String(PAGE_COUNT));
    drawPage(currentPage, true);
  } else if (downReady) {
    currentPage = (currentPage + 1) % PAGE_COUNT;
    Serial.println("DOWN -> page " + String(currentPage + 1) + "/" + String(PAGE_COUNT));
    drawPage(currentPage, true);
  } else if (menuReady) {
    currentPage = 0;
    Serial.println("MENU -> home (page 1/" + String(PAGE_COUNT) + ")");
    drawPage(currentPage, true);
  } else if (exitReady) {
    Serial.println("EXIT -> full refresh of page " + String(currentPage + 1));
    drawPage(currentPage, false);  // full (non-partial) refresh clears e-paper ghosting
  }
}

void drawPage(int page, bool partial) {
  epd.fillScreen(GxEPD_WHITE);
  epd.setTextColor(GxEPD_BLACK);
  epd.setFont(&FreeSans9pt7b);
  epd.setCursor(LEFT_MARGIN, 32);  // clears the 24pt header's tall ascent on page 1 without hurting other pages

  switch (page) {
    case 0: printChipInfo(); break;
    case 1: printMemoryInfo(); break;
    case 2: printEpaperPins(); break;
    case 3: printButtonPins(); break;
    case 4: printSdCardPins(); break;
    case 5: printHeaderPins(); break;
    case 6: printResetAndUptime(); break;
    case 7: printBuildInfo(); break;
    case 8: printTemperature(); break;
    case 9: printChipFeatures(); break;
    case 10: printPartitionTable(); break;
    case 11: printWifiScan(); break;
  }

  epd.setCursor(LEFT_MARGIN, epd.height() - 10);
  epd.print("Page ");
  epd.print(page + 1);
  epd.print("/");
  epd.print(PAGE_COUNT);

  epd.display(partial);
}

// Adafruit_GFX resets cursor_x to 0 on every newline (confirmed in
// Adafruit_GFX.cpp's write()), regardless of font -- so anything printed via
// println() drifts to the very left edge after the first line unless the
// margin is re-applied before each one. Called at the start of every
// print*()/printLine() below rather than relying on the caller to remember.
void resetLeftMargin() {
  epd.setCursor(LEFT_MARGIN, epd.getCursorY());
}

// Shared label/value row used by nearly every page. The label prints bold
// so it stands out from its value at a glance in the middle of a dense list.
void printLine(const char *label, const String &value) {
  resetLeftMargin();
  epd.setFont(&FreeSansBold9pt7b);
  epd.print(label);        // make bold
  epd.setFont(&FreeSans9pt7b);
  epd.print(": ");
  epd.println(value);
}

// Larger font for a page's section title, then back to body size for
// whatever printLine() calls follow -- replaces the old "--- Title ---"
// plain-text convention now that font size itself marks it as a header.
void printTitle(const char *title) {
  resetLeftMargin();
  epd.setFont(&FreeSans12pt7b);
  epd.println(title);
  epd.setFont(&FreeSans9pt7b);
}

void printHeader(const char *header) {
  resetLeftMargin();
  epd.setFont(&FreeSerifBoldItalic24pt7b);
  epd.println(header);
  epd.setFont(&FreeSans9pt7b);
}

String macBytesToString(const uint8_t mac[6]) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

// esp_read_mac() derives whichever MAC is asked for straight from eFuse --
// unlike WiFi.macAddress(), it doesn't need the radio initialized first
// (which, unrequested here, would just read back as all zeros).
String wifiStationMacToString() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  return macBytesToString(mac);
}

void printChipInfo() {
  printHeader("CrowPanel");
  printTitle("Chip");
  printLine("Model", ESP.getChipModel());
  printLine("Revision", String(ESP.getChipRevision()));
  printLine("Cores", String(ESP.getChipCores()));
  printLine("CPU freq (MHz)", String(ESP.getCpuFreqMHz()));
  printLine("Flash size (bytes)", String(ESP.getFlashChipSize()));
  printLine("Flash speed (Hz)", String(ESP.getFlashChipSpeed()));
  printLine("SDK version", ESP.getSdkVersion());
  printLine("MAC address", wifiStationMacToString());
}

void printMemoryInfo() {
  printTitle("Memory");
  printLine("Heap size (bytes)", String(ESP.getHeapSize()));
  printLine("Free heap (bytes)", String(ESP.getFreeHeap()));
  printLine("Min free heap seen (bytes)", String(ESP.getMinFreeHeap()));
  // PSRAM must be enabled in board options (PSRAM=opi) or these read as 0/absent.
  printLine("PSRAM size (bytes)", String(ESP.getPsramSize()));
  printLine("Free PSRAM (bytes)", String(ESP.getFreePsram()));
}

void printEpaperPins() {
  printTitle("E-Paper Panel (SPI)");
  printLine("PWR (power enable)", "GPIO7");
  printLine("CS (chip select)", "GPIO45");
  printLine("DC (data/command)", "GPIO46");
  printLine("RST (reset)", "GPIO47");
  printLine("BUSY", "GPIO48");
  printLine("SCK [Elecrow-published, unconfirmed]", "GPIO12");
  printLine("MOSI [Elecrow-published, unconfirmed]", "GPIO11");
}

// The rotary switch is one physical knob (rotate = up/down, press = OK), not
// three separate buttons -- drawn as a circle with rotate arrows and a
// center press-dot so it isn't visually confused with MENU/EXIT's plain
// push-buttons below/above it on the diagram.
void drawRotaryIcon(int cx, int cy) {
  const int r = 10;
  epd.drawCircle(cx, cy, r, GxEPD_BLACK);
  epd.fillCircle(cx, cy, 2, GxEPD_BLACK);
  epd.fillTriangle(cx - 4, cy - r - 2, cx + 4, cy - r - 2, cx, cy - r - 8, GxEPD_BLACK);
  epd.fillTriangle(cx - 4, cy + r + 2, cx + 4, cy + r + 2, cx, cy + r + 8, GxEPD_BLACK);
}

// Real physical layout from Elecrow's own hardware-overview photo
// (elecrow.com product page for DIE07300S): MENU, the rotary switch, and
// EXIT sit in that top-to-bottom order along the board's left edge. Drawn
// here as a schematic (not to scale/position-perfect), not copied pixel for
// pixel from the photo -- just matching the real order and edge placement.
void drawButtonDiagram() {
  const int edgeX = 20;
  const int menuY = 80;
  const int rotaryY = 115;
  const int exitY = 150;

  epd.drawLine(edgeX, 65, edgeX, 165, GxEPD_BLACK);  // the device's left edge

  epd.drawCircle(edgeX, menuY, 6, GxEPD_BLACK);
  epd.setCursor(edgeX + 20, menuY + 4);
  epd.print("MENU");

  drawRotaryIcon(edgeX, rotaryY);
  epd.setCursor(edgeX + 20, rotaryY + 4);
  epd.print("Rotary");

  epd.drawCircle(edgeX, exitY, 6, GxEPD_BLACK);
  epd.setCursor(edgeX + 20, exitY + 4);
  epd.print("EXIT");
}

void printButtonPins() {
  printTitle("Buttons / Rotary");
  drawButtonDiagram();
  epd.setCursor(LEFT_MARGIN, 185);  // resume the text list below the diagram
  printLine("MENU (GPIO2)", "-> jump to page 1");
  printLine("EXIT (GPIO1)", "-> full refresh this page");
  printLine("Rotary UP (GPIO6)", "-> previous page");
  printLine("Rotary DOWN (GPIO4)", "-> next page");
  printLine("Rotary OK/Config", "(GPIO5), reserved");
  // BOOT/RESET (top edge, per the hardware photo) aren't app-readable GPIOs
  // like the five above -- BOOT selects flashing mode at power-on, RESET
  // hard-resets the chip -- so they're listed but not part of the diagram.
  printLine("BOOT", "top edge, flashing mode only");
  printLine("RESET", "top edge, hardware reset only");
}

void printSdCardPins() {
  printTitle("SD Card (SPI)");
  printLine("CS", "GPIO10");
  printLine("SCK", "GPIO39");
  printLine("MOSI", "GPIO40");
  printLine("MISO", "GPIO13");
}

// ADC/touch channel numbers and the RTC/digital-only split come from the
// ESP32-S3 datasheet's pin table, not this board's own docs -- true for any
// ESP32-S3 module, not specific to the CrowPanel wiring. Touch is folded into
// this same page rather than a separate one, since it's the same 12 pins --
// on the S3, touch channels map 1:1 to GPIO1-14, so only GPIO3/8/9/14 here
// qualify; GPIO15 and up have no touch capability.
void printHeaderPins() {
  printTitle("GP Header");
  printLine("GPIO3", "ADC1_2, Touch3, strap pin (boot)");
  printLine("GPIO8", "ADC1_7, Touch8");
  printLine("GPIO9", "ADC1_8, Touch9");
  printLine("GPIO14", "ADC2_3, Touch14");
  printLine("GPIO15", "ADC2_4");
  printLine("GPIO16", "ADC2_5");
  printLine("GPIO17", "ADC2_6");
  printLine("GPIO18", "ADC2_7");
  printLine("GPIO19", "ADC2_8, USB D- (unused, CH340 used instead)");
  printLine("GPIO20", "ADC2_9, USB D+ (unused, CH340 used instead)");
  printLine("GPIO21", "RTC GPIO, no ADC/touch");
  printLine("GPIO38", "digital only, no ADC/RTC/touch");
}

// esp_reset_reason() names come from the stable subset of esp_reset_reason_t
// across recent ESP-IDF versions; a handful of newer/rarer causes (USB, JTOG,
// efuse error, power glitch) fall through to the numeric default case below
// rather than being individually named.
String resetReasonToString(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "Power-on";
    case ESP_RST_EXT: return "External pin";
    case ESP_RST_SW: return "Software (esp_restart)";
    case ESP_RST_PANIC: return "Software panic/crash";
    case ESP_RST_INT_WDT: return "Interrupt watchdog";
    case ESP_RST_TASK_WDT: return "Task watchdog";
    case ESP_RST_WDT: return "Other watchdog";
    case ESP_RST_DEEPSLEEP: return "Wake from deep sleep";
    case ESP_RST_BROWNOUT: return "Brownout (voltage dip)";
    case ESP_RST_SDIO: return "SDIO";
    default: return "Other/unknown (code " + String((int)reason) + ")";
  }
}

void printResetAndUptime() {
  printTitle("Reset / Uptime");
  printLine("Last reset reason", resetReasonToString(esp_reset_reason()));

  unsigned long totalSeconds = millis() / 1000;
  unsigned long hours = totalSeconds / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;
  char uptimeBuf[16];
  snprintf(uptimeBuf, sizeof(uptimeBuf), "%02lu:%02lu:%02lu", hours, minutes, seconds);
  printLine("Uptime:", uptimeBuf);
}

// FM_QIO/FM_QOUT/etc. name the flash chip's read mode (quad vs dual I/O,
// fast vs slow read) -- set by the board package's boot header, not chosen
// at runtime by this sketch.
String flashModeToString(FlashMode_t mode) {
  switch (mode) {
    case FM_QIO: return "QIO (quad I/O)";
    case FM_QOUT: return "QOUT (quad output)";
    case FM_DIO: return "DIO (dual I/O)";
    case FM_DOUT: return "DOUT (dual output)";
    case FM_FAST_READ: return "Fast read";
    case FM_SLOW_READ: return "Slow read";
    default: return "Unknown";
  }
}

void printBuildInfo() {
  printTitle("Sketch / Flash");
  printLine("Sketch size (bytes)", String(ESP.getSketchSize()));
  printLine("Free sketch space", String(ESP.getFreeSketchSpace()));
  printLine("Sketch MD5", ESP.getSketchMD5());
  printLine("Flash mode", flashModeToString(ESP.getFlashChipMode()));
}

void printTemperature() {
  printTitle("Die Temperature");
  // temperatureRead() reads the ESP32-S3's internal die sensor -- reflects
  // chip temperature (which rises under load), not ambient room temperature.
  float celsius = temperatureRead();
  float fahrenheit = celsius * 9.0 / 5.0 + 32.0;
  printLine("Chip temp (C)", String(celsius, 1));
  printLine("Chip temp (F)", String(fahrenheit, 1));
}

void printChipFeatures() {
  printTitle("Radios / Features");
  esp_chip_info_t info;
  esp_chip_info(&info);
  printLine("Embedded flash", (info.features & CHIP_FEATURE_EMB_FLASH) ? "yes" : "no");
  printLine("Embedded PSRAM", (info.features & CHIP_FEATURE_EMB_PSRAM) ? "yes" : "no");
  printLine("WiFi 802.11 b/g/n", (info.features & CHIP_FEATURE_WIFI_BGN) ? "yes" : "no");
  printLine("Bluetooth Classic", (info.features & CHIP_FEATURE_BT) ? "yes" : "no");
  printLine("Bluetooth LE", (info.features & CHIP_FEATURE_BLE) ? "yes" : "no");

  uint8_t btMac[6];
  esp_read_mac(btMac, ESP_MAC_BT);
  printLine("Bluetooth MAC", macBytesToString(btMac));
}

void printPartitionTable() {
  printTitle("Flash Partitions");
  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  for (; it != NULL; it = esp_partition_next(it)) {
    const esp_partition_t *p = esp_partition_get(it);
    printLine(p->label, String(p->size / 1024) + " KB @ 0x" + String(p->address, HEX));
  }
  esp_partition_iterator_release(it);
}

void printWifiScan() {
  printTitle("WiFi Networks");
  // Only page that touches the radio -- costs a few seconds per visit to
  // this page, unlike every other page's near-instant redraw.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int found = WiFi.scanNetworks();

  if (found <= 0) {
    printLine("Found", "0 networks");
  } else {
    int shown = min(found, 10);  // cap so the list doesn't run off-screen
    for (int i = 0; i < shown; i++) {
      printLine(WiFi.SSID(i).c_str(), String(WiFi.RSSI(i)) + " dBm");
    }
    if (found > shown) {
      resetLeftMargin();
      epd.println("(+" + String(found - shown) + " more not shown)");
    }
  }
  WiFi.scanDelete();
}
