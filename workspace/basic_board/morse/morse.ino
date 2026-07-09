// WiFi Morse code blinker for ESP32.
// Blinks a message out on the onboard LED in Morse code, over and over,
// starting with a built-in default ("HELLO WORLD"). A small web page hosted
// on the local network lets you replace the message from a browser; the new
// message takes over at the next symbol boundary.

#include <WiFi.h>
#include <WebServer.h>
#include "wifi_config.h"

// Most ESP32 DevKitC/WROOM-32 boards wire the onboard LED to GPIO2.
// If nothing blinks, check your board's pinout and change this.
const int LED_PIN = 2;

// Morse timing is all derived from one unit (the dot length) per the
// standard: dash = 3 units, inter-symbol gap = 1 unit, inter-letter gap =
// 3 units, inter-word gap = 7 units.
const int UNIT_MS = 200;
const int DOT_MS = UNIT_MS;
const int DASH_MS = UNIT_MS * 3;
const int SYMBOL_GAP_MS = UNIT_MS;
const int LETTER_GAP_MS = UNIT_MS * 3;
const int WORD_GAP_MS = UNIT_MS * 7;

const char *DEFAULT_MESSAGE = "Hello World";

WebServer server(80);

String currentMessage = DEFAULT_MESSAGE;
// Set by the web handler when a new message arrives; the blink loop checks
// it between symbols so a fresh message interrupts the current pass instead
// of waiting for the whole thing to finish.
bool messageChanged = false;

struct MorseEntry {
  char letter;
  const char *code;
};

// A-Z and 0-9 International Morse Code reference table.
const MorseEntry MORSE_TABLE[] = {
    {'A', ".-"},    {'B', "-..."},  {'C', "-.-."},  {'D', "-.."},
    {'E', "."},     {'F', "..-."},  {'G', "--."},   {'H', "...."},
    {'I', ".."},    {'J', ".---"},  {'K', "-.-"},   {'L', ".-.."},
    {'M', "--"},    {'N', "-."},    {'O', "---"},   {'P', ".--."},
    {'Q', "--.-"},  {'R', ".-."},   {'S', "..."},   {'T', "-"},
    {'U', "..-"},   {'V', "...-"},  {'W', ".--"},   {'X', "-..-"},
    {'Y', "-.--"},  {'Z', "--.."},  {'0', "-----"}, {'1', ".----"},
    {'2', "..---"}, {'3', "...--"}, {'4', "....-"}, {'5', "....."},
    {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."},
};

// Case-insensitive: Morse has no letter case, so messages keep whatever
// casing they were typed with and the lookup folds to uppercase here.
const char *codeForLetter(char letter) {
  char upper = toupper(letter);
  for (const MorseEntry &entry : MORSE_TABLE) {
    if (entry.letter == upper) {
      return entry.code;
    }
  }
  return nullptr;
}

unsigned long lastHeartbeatMs = 0;
const unsigned long HEARTBEAT_INTERVAL_MS = 5000;

// setup() doesn't block waiting for WiFi, so this is where "connected" is
// first noticed and the web server actually brought up.
bool serverStarted = false;

// Services the web server plus the periodic heartbeat log line. Called from
// every wait in this sketch (not just loop()), so HTTP requests get answered
// even in the middle of a blink pass.
void serviceNetwork() {
  if (!serverStarted && WiFi.status() == WL_CONNECTED) {
    serverStarted = true;
    Serial.print("Connected. Open http://");
    Serial.println(WiFi.localIP());
    server.begin();
  }
  if (serverStarted) {
    server.handleClient();
  }

  unsigned long now = millis();
  if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeatMs = now;
    Serial.printf("[heartbeat] uptime=%lus wifi=%s ip=%s\n", now / 1000,
                  WiFi.status() == WL_CONNECTED ? "connected" : "disconnected",
                  WiFi.localIP().toString().c_str());
  }
}

// delay() replacement that keeps answering HTTP requests in small slices
// instead of going dark for the whole wait.
void serverDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    serviceNetwork();
    delay(5);
  }
}

void blinkSymbol(char symbol) {
  digitalWrite(LED_PIN, HIGH);
  serverDelay(symbol == '-' ? DASH_MS : DOT_MS);
  digitalWrite(LED_PIN, LOW);
  serverDelay(SYMBOL_GAP_MS);
}

void blinkLetter(char letter) {
  const char *code = codeForLetter(letter);
  if (code == nullptr) {
    // Unsupported character (punctuation, etc.) — skip it rather than fail.
    return;
  }
  for (const char *symbol = code; *symbol != '\0' && !messageChanged; symbol++) {
    blinkSymbol(*symbol);
  }
}

// Takes a copy (not a reference) on purpose: the web handler runs inside
// serverDelay() and may reassign currentMessage mid-pass, and iterating a
// String while it's replaced under us would be asking for trouble. The
// messageChanged checks are what cut the pass short once that happens.
void blinkMessage(String message) {
  for (size_t i = 0; i < message.length() && !messageChanged; i++) {
    char c = message[i];
    if (c == ' ') {
      serverDelay(WORD_GAP_MS);
    } else {
      blinkLetter(c);
      serverDelay(LETTER_GAP_MS);
    }
  }
}

// Escapes the handful of characters that matter when echoing user input
// back into an HTML response, so a message can't inject markup.
String escapeHtml(const String &input) {
  String out;
  out.reserve(input.length());
  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out += c; break;
    }
  }
  return out;
}

const char *PAGE_TEMPLATE =
    "<!DOCTYPE html><html><head><title>ESP32 Morse Blinker</title></head>"
    "<body style=\"font-family:sans-serif;max-width:32em;margin:2em auto\">"
    "<h1>ESP32 Morse Blinker</h1>"
    "%s"
    "<form method=\"POST\" action=\"/blink\">"
    "<input type=\"text\" name=\"message\" placeholder=\"Type a new message\" "
    "maxlength=\"120\" autofocus>"
    "<button type=\"submit\">Blink it</button>"
    "</form>"
    "</body></html>";

void sendPage(const String &statusHtml) {
  char buffer[2048];
  snprintf(buffer, sizeof(buffer), PAGE_TEMPLATE, statusHtml.c_str());
  server.send(200, "text/html", buffer);
}

void handleRoot() {
  Serial.println("[request] GET /");
  sendPage("<p>Currently blinking: <strong>" + escapeHtml(currentMessage) +
           "</strong></p><p>Type a new message below to replace it.</p>");
}

void handleBlink() {
  String message = server.arg("message");
  Serial.println("[request] POST /blink: " + message);

  if (message.length() == 0) {
    sendPage("<p>Empty message ignored — still blinking: <strong>" +
             escapeHtml(currentMessage) + "</strong></p>"
             "<p><a href=\"/\">Back</a></p>");
    return;
  }

  currentMessage = message;
  messageChanged = true;  // cuts the in-progress blink pass short

  sendPage("<p>Now blinking: <strong>" + escapeHtml(currentMessage) +
           "</strong></p><p><a href=\"/\">Back</a></p>");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // Fire-and-forget: don't block boot on the network. The LED starts
  // blinking the default message right away; serviceNetwork() brings the
  // web server up whenever the connection actually lands.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("WiFi connecting in background...");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/blink", HTTP_POST, handleBlink);
  server.onNotFound(handleNotFound);
}

void loop() {
  serviceNetwork();

  messageChanged = false;
  Serial.println("[blink] " + currentMessage);
  blinkMessage(currentMessage);

  // Pause between repeats — skipped when a new message just arrived, so it
  // starts promptly instead of sitting through a word gap first.
  if (!messageChanged) {
    serverDelay(WORD_GAP_MS);
  }
}
