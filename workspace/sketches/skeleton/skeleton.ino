// --- Pin lifecycle skeleton: button-triggered e-paper draw ---
//
// Illustrative reference only -- not a working sketch. `...` placeholders
// (display template args, epd.init() args, drawing calls) are intentionally
// left unfilled. The point is the *shape* of the pattern: one struct+function
// pair per button, one power/init/draw/display/hibernate/power-off bracket
// per redraw, nothing outside that bracket.

#define PWR   7   // panel power MOSFET
#define BUSY  48
#define RES   47
#define DC    46
#define CS    45
#define BTN   2   // whichever button this is (MENU, EXIT, etc.)

GxEPD2_BW<...> epd(...);

struct Button {
  int pin;
  bool wasPressed = false;
  unsigned long lastActionMs = 0;
};

Button btn = {BTN};

// Returns true exactly once per clean press (edge-detected + debounced).
bool pressedAndReady(Button &b, unsigned long now) {
  bool pressed = digitalRead(b.pin) == LOW;   // active-low, INPUT_PULLUP
  bool edge = pressed && !b.wasPressed;
  b.wasPressed = pressed;

  bool ready = edge && (now - b.lastActionMs) > COOLDOWN_MS;
  if (ready) b.lastActionMs = now;
  return ready;
}

void setup() {
  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, HIGH);      // power on once at boot
  pinMode(BTN, INPUT_PULLUP);
  epd.init(...);                // init once at boot
  // panel can be left powered, or cut here if you want idle = unpowered
}

void loop() {
  if (pressedAndReady(btn, millis())) {
    // --- the redraw bracket: everything the panel needs happens here ---
    digitalWrite(PWR, HIGH);    // re-power if you cut it after the last draw
    // epd.init(...) again too, IF your display/library needs it after
    // power was fully cut -- not always true, verify on real hardware.

    // ... build the frame: fillScreen / setCursor / print / drawX calls ...

    epd.display();              // blocking: waits for physical refresh
    epd.hibernate();             // controller sleeps
    digitalWrite(PWR, LOW);     // cut power until the next press
  }
  // idle the rest of the time -- no drawing, no power, just polling one pin
}
